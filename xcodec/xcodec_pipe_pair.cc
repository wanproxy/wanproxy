/*
 * Copyright (c) 2010-2014 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common/buffer.h>
#include <common/endian.h>

#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_pair.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>
#include <xcodec/xcodec_pipe_pair.h>
#include <xcodec/xcodec_pipe_protocol.h>

/*
 * XXX
 * Especially now that we support disk storage of data, we need to better
 * handle the case of collisions and such.  One easy piece would be to
 * include a checksum in each frame, and to check the decoded frame.  If it
 * is inconsistent, then the sender can retry with smaller and smaller
 * frame sizes until we simply get escaped data.  Easy to implement, and
 * robust in the face of failure.  Could mean sending quite a lot of data
 * to make progress, though.  We could raise and lower the window like TCP
 * does, to improve throughput.
 *
 * We also then need to evict old hashes on collision in decode, since we
 * want to be able to recover long-term, and not have constantly-degraded
 * performance due to a common hash being renamed.
 */

/*
 * Allow up to 1MB of data per encoded frame.
 */
#define	XCODEC_PIPE_MAX_FRAME	(1024 * 1024)

/*
 * Allow up to 512 items per ASK/LEARN.
 */
#define	XCODEC_PIPE_ASK_MAX	(512)

void
XCodecPipePair::decoder_consume(Buffer *buf)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (buf->empty()) {
		if (!decoder_buffer_.empty())
			ERROR(log_) << "Remote encoder closed connection with data outstanding.";
		if (!decoder_frame_buffer_.empty())
			ERROR(log_) << "Remote encoder closed connection with frame data outstanding.";
		if (!decoder_sent_eos_) {
			DEBUG(log_) << "Decoder received, sent EOS.";
			decoder_sent_eos_ = true;
			decoder_produce_eos();
		} else {
			DEBUG(log_) << "Decoder received EOS after sending EOS.";
		}
		return;
	}

	buf->moveout(&decoder_buffer_);

	/*
	 * While we process data, we need to cork the encoder in case we generate any
	 * output as a result.
	 */
	encoder_pipe_->cork();

	/*
	 * Decode as much data as we can.
	 */
	if (!decoder_decode()) {
		decoder_error();
		encoder_pipe_->uncork();
		return;
	}

	/*
	 * Decode any frames we extracted and process the data stream
	 * state, unless we're still waiting for a <LEARN>.
	 */
	if (decoder_unknown_hashes_.empty()) {
		if (!decoder_decode_data()) {
			decoder_error();
			encoder_pipe_->uncork();
			return;
		}
	}

	/*
	 * If we have more data to decode, do not fall through to the EOS
	 * checks that follow, but we're clearly waiting for more data.
	 */
	if (!decoder_buffer_.empty() || !decoder_frame_buffer_.empty()) {
		encoder_pipe_->uncork();
		return;
	}

	/*
	 * If we have received EOS and not yet sent it, we can send it now.
	 * The only caveat is that if we have outstanding <ASK>s, i.e. we have
	 * not yet emptied decoder_unknown_hashes_, then we can't send EOS yet.
	 */
	if (decoder_received_eos_ && !decoder_sent_eos_) {
		ASSERT(log_, !decoder_sent_eos_);
		if (decoder_unknown_hashes_.empty()) {
			ASSERT(log_, decoder_frame_buffer_.empty());
			DEBUG(log_) << "Decoder finished, got <EOS>, shutting down decoder output channel.";
			decoder_produce_eos();
			decoder_sent_eos_ = true;
		} else {
			ASSERT(log_, !decoder_frame_buffer_.empty());
			DEBUG(log_) << "Decoder finished, waiting to send <EOS> until <ASK>s are answered.";
		}
	}

	/*
	 * NB:
	 * Along with the comment above, there is some relevance here.  If we
	 * use some kind of hierarchical decoding, then we need to be able to
	 * handle the case where an <ASK>'s response necessitates us to send
	 * another <ASK> or something of that sort.  There are other conditions
	 * where we may still need to send something out of the encoder, but
	 * thankfully none seem to arise yet.
	 */
	if (encoder_sent_eos_ack_ && decoder_received_eos_ack_) {
		ASSERT(log_, decoder_buffer_.empty());
		ASSERT(log_, decoder_frame_buffer_.empty());

		DEBUG(log_) << "Decoder finished, got <EOS_ACK>, shutting down encoder output channel.";

		ASSERT(log_, !encoder_produced_eos_);
		encoder_produce_eos();
		encoder_produced_eos_ = true;
	}

	/*
	 * Done processing data, uncork.
	 */
	encoder_pipe_->uncork();
}

bool
XCodecPipePair::decoder_decode(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	while (!decoder_buffer_.empty()) {
		uint8_t op = decoder_buffer_.peek();
		switch (op) {
		case XCODEC_PIPE_OP_HELLO:
			if (decoder_cache_ != NULL) {
				ERROR(log_) << "Got <HELLO> twice.";
				return (false);
			} else {
				uint8_t len;
				if (decoder_buffer_.length() < sizeof op + sizeof len)
					return (true);
				decoder_buffer_.extract(&len, sizeof op);

				if (decoder_buffer_.length() < sizeof op + sizeof len + len)
					return (true);

				if (len != UUID_SIZE) {
					ERROR(log_) << "Unsupported <HELLO> length: " << (unsigned)len;
					return (false);
				}

				Buffer uubuf;
				decoder_buffer_.moveout(&uubuf, sizeof op + sizeof len, UUID_SIZE);

				UUID uuid;
				if (!uuid.decode(&uubuf)) {
					ERROR(log_) << "Invalid UUID in <HELLO>.";
					return (false);
				}

				decoder_cache_ = XCodecCache::connect(uuid, codec_->cache());
				ASSERT_NULL(log_, decoder_);
				decoder_ = new XCodecDecoder(decoder_cache_);

				DEBUG(log_) << "Peer connected with UUID: " << uuid.string_;
			}
			break;
		case XCODEC_PIPE_OP_ASK:
			if (encoder_ == NULL) {
				ERROR(log_) << "Got <ASK> before sending <HELLO>.";
				return (false);
			} else {
				uint16_t becount;
				if (decoder_buffer_.length() < sizeof op + sizeof becount)
					return (true);
				decoder_buffer_.extract(&becount, sizeof op);

				uint16_t count = BigEndian::decode(becount);
				if (count == 0 || count > XCODEC_PIPE_ASK_MAX) {
					ERROR(log_) << "Got invalid count for <ASK>: " << count;
					return (false);
				}

				uint64_t hash;
				if (decoder_buffer_.length() < sizeof op + sizeof count + (sizeof hash * count))
					return (true);

				decoder_buffer_.skip(sizeof op + sizeof count);

				Buffer learn;
				learn.append(XCODEC_PIPE_OP_LEARN);
				learn.append(&becount);
				while (count-- != 0) {
					decoder_buffer_.moveout(&hash);
					hash = BigEndian::decode(hash);

					if (encoder_reference_frames_.empty()) {
						ERROR(log_) << "Got <ASK> when all encoded frames have been processed.";
						return (false);
					}

					/*
					 * XXX
					 * It seems like we should only get an <ASK> for
					 * the topmost frame, because <ADVANCE> is sent
					 * out ahead of <ASK> when processing multiple
					 * frames.  Mind, we process one frame at a time
					 * these days.
					 */
					std::list<std::map<uint64_t, BufferSegment *> *>::const_iterator rmit;
					for (rmit = encoder_reference_frames_.begin();
					     rmit != encoder_reference_frames_.end(); ++rmit) {
						std::map<uint64_t, BufferSegment *> *refmap = *rmit;

						std::map<uint64_t, BufferSegment *>::const_iterator it;
						it = refmap->find(hash);
						if (it == refmap->end())
							continue;

						learn.append(it->second);
						break;
					}
					if (rmit == encoder_reference_frames_.end()) {
						ERROR(log_) << "Hash in <ASK> could not be found in any reference frame: "<< hash;
						return (false);
					}
				}
				DEBUG(log_) << "Responding to <ASK> with <LEARN>.";
				encoder_produce(&learn);
			}
			break;
		case XCODEC_PIPE_OP_LEARN:
			if (decoder_cache_ == NULL) {
				ERROR(log_) << "Got <LEARN> before <HELLO>.";
				return (false);
			} else {
				uint16_t count;
				if (decoder_buffer_.length() < sizeof op + sizeof count)
					return (true);
				decoder_buffer_.extract(&count, sizeof op);

				count = BigEndian::decode(count);
				if (count == 0 || count > XCODEC_PIPE_ASK_MAX) {
					ERROR(log_) << "Got invalid count for <LEARN>: " << count;
					return (false);
				}

				if (decoder_buffer_.length() < sizeof op + sizeof count + (XCODEC_SEGMENT_LENGTH * count))
					return (true);

				decoder_buffer_.skip(sizeof op + sizeof count);

				while (count-- != 0) {
					BufferSegment *seg;
					decoder_buffer_.copyout(&seg, XCODEC_SEGMENT_LENGTH);
					decoder_buffer_.skip(XCODEC_SEGMENT_LENGTH);

					uint64_t hash = XCodecHash::hash(seg->data());
					if (decoder_unknown_hashes_.find(hash) == decoder_unknown_hashes_.end()) {
						/*
						 * XXX
						 * This can happen if we send a duplicate <ASK>.
						 * Have we weeded all such cases out?
						 */
						ERROR(log_) << "Gratuitous <LEARN> without <ASK>.";
						return (false);
					}
					decoder_unknown_hashes_.erase(hash);

					BufferSegment *oseg = decoder_cache_->lookup(hash);
					if (oseg != NULL) {
						if (oseg->equal(seg)) {
							oseg->unref();
							DEBUG(log_) << "Redundant <LEARN>.";
						} else {
							/*
							 * See note about allowing name reuse in
							 * <EXTRACT> handling in the decoder.
							 */
							INFO(log_) << "Name reuse in <LEARN>.";
							oseg->unref();
							decoder_cache_->replace(hash, seg);
						}
					} else {
						decoder_cache_->enter(hash, seg);
					}
					seg->unref();
				}
				DEBUG(log_) << "Successful <LEARN>.";
			}
			break;
		case XCODEC_PIPE_OP_EOS:
			if (decoder_received_eos_) {
				ERROR(log_) << "Duplicate <EOS>.";
				return (false);
			}
			decoder_buffer_.skip(1);
			decoder_received_eos_ = true;
			break;
		case XCODEC_PIPE_OP_EOS_ACK:
			if (!encoder_sent_eos_) {
				ERROR(log_) << "Got <EOS_ACK> before sending <EOS>.";
				return (false);
			}
			if (decoder_received_eos_ack_) {
				ERROR(log_) << "Duplicate <EOS_ACK>.";
				return (false);
			}
			decoder_buffer_.skip(1);
			decoder_received_eos_ack_ = true;
			break;
		case XCODEC_PIPE_OP_FRAME:
			if (decoder_ == NULL) {
				ERROR(log_) << "Got frame data before decoder initialized.";
				return (false);
			} else {
				uint32_t len;
				if (decoder_buffer_.length() < sizeof op + sizeof len)
					return (true);
				decoder_buffer_.extract(&len, sizeof op);
				len = BigEndian::decode(len);
				if (len == 0 || len > XCODEC_PIPE_MAX_FRAME) {
					ERROR(log_) << "Invalid framed data length.";
					return (false);
				}

				if (decoder_buffer_.length() < sizeof op + sizeof len + len)
					return (true);

				/*
				 * XXX
				 * If we don't merge frames for decode, we can simplify
				 * the logic around decoder_frame_lenghts_ and <ASK>/<LEARN>
				 * so that we never have to buffer more than a frame at
				 * a time while waiting for a <LEARN> whereas right now a
				 * single <LEARN> will delay all production from the
				 * decoder.
				 */
				decoder_buffer_.moveout(&decoder_frame_buffer_, sizeof op + sizeof len, len);

				/*
				 * Track the length of each frame in a vector so
				 * that we can tell below how many frames we've
				 * finished with, for the sake of OP_ADVANCE.
				 */
				decoder_frame_lengths_.push_back(len);
			}
			break;
		case XCODEC_PIPE_OP_ADVANCE:
			if (encoder_ == NULL) {
				ERROR(log_) << "Got <ADVANCE> before sending <HELLO>.";
				return (false);
			} else {
				uint32_t count;
				if (decoder_buffer_.length() < sizeof op + sizeof count)
					return (true);
				decoder_buffer_.moveout(&count, sizeof op);
				count = BigEndian::decode(count);

				if (count == 0 || count > encoder_reference_frames_.size()) {
					ERROR(log_) << "Invalid frame advance count.";
					return (false);
				}

				while (count != 0) {
					encoder_reference_frame_advance();
					count--;
				}
			}
			break;
		default:
			ERROR(log_) << "Unsupported operation in pipe stream.";
			return (false);
		}
	}

	return (true);
}

/*
 * Decode the actual framed data.
 */
bool
XCodecPipePair::decoder_decode_data(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT(log_, decoder_unknown_hashes_.empty());

	if (decoder_frame_buffer_.empty()) {
		if (decoder_received_eos_ && !encoder_sent_eos_ack_) {
			DEBUG(log_) << "Decoder finished, got <EOS>, sending <EOS_ACK>.";

			Buffer eos_ack;
			eos_ack.append(XCODEC_PIPE_OP_EOS_ACK);

			encoder_produce(&eos_ack);
			encoder_sent_eos_ack_ = true;
		}
		return (true);
	}

	size_t frame_buffer_consumed = decoder_frame_buffer_.length();
	Buffer output;
	if (!decoder_->decode(&output, &decoder_frame_buffer_, decoder_unknown_hashes_)) {
		ERROR(log_) << "Decoder exiting with error.";
		return (false);
	}

	frame_buffer_consumed -= decoder_frame_buffer_.length();
	if (frame_buffer_consumed != 0) {
		uint32_t frame_buffer_advance = 0;
		for (;;) {
			std::list<uint32_t>::iterator dflit;
			dflit = decoder_frame_lengths_.begin();

			size_t first_frame_length = *dflit;
			if (frame_buffer_consumed < first_frame_length) {
				*dflit = first_frame_length - frame_buffer_consumed;
				break;
			}
			frame_buffer_consumed -= first_frame_length;
			frame_buffer_advance++;

			decoder_frame_lengths_.pop_front();

			if (frame_buffer_consumed == 0)
				break;
		}
		if (frame_buffer_advance != 0) {
			Buffer advance;

			advance.append(XCODEC_PIPE_OP_ADVANCE);
			frame_buffer_advance = BigEndian::encode(frame_buffer_advance);
			advance.append(&frame_buffer_advance);
			encoder_produce(&advance);
		}
	}

	if (!output.empty()) {
		ASSERT(log_, !decoder_sent_eos_);
		decoder_produce(&output);
	} else {
		/*
		 * We should only get no output from the decoder if
		 * we're waiting on the next frame or we need an
		 * unknown hash.  It would be nice to make the
		 * encoder framing aware so that it would not end
		 * up with encoded data that straddles a frame
		 * boundary.  (Fixing that would also allow us to
		 * simplify length checking within the decoder
		 * considerably.)
		 */
		ASSERT(log_, !decoder_frame_buffer_.empty() || !decoder_unknown_hashes_.empty());
	}

	/*
	 * We've processed the data and are done.
	 */
	if (decoder_unknown_hashes_.empty())
		return (true);

	/*
	 * Send <ASK>s in groups of XCODEC_PIPE_ASK_MAX.
	 */
	Buffer ask;
	std::set<uint64_t>::const_iterator it;
	unsigned hashcnt = decoder_unknown_hashes_.size();
	unsigned nhash = 0;
	for (it = decoder_unknown_hashes_.begin(); it != decoder_unknown_hashes_.end(); ++it) {
		uint64_t hash = *it;
		hash = BigEndian::encode(hash);

		if (nhash == 0) {
			uint16_t count;
			if (hashcnt > XCODEC_PIPE_ASK_MAX) {
				count = XCODEC_PIPE_ASK_MAX;
				hashcnt -= XCODEC_PIPE_ASK_MAX;
			} else {
				count = hashcnt;
				hashcnt = 0;
			}
			count = BigEndian::encode(count);

			ASSERT(log_, ask.empty());
			ask.append(XCODEC_PIPE_OP_ASK);
			ask.append(&count);
		}
		ASSERT(log_, !ask.empty());
		ask.append(&hash);
		if (++nhash == XCODEC_PIPE_ASK_MAX) {
			nhash = 0;
			DEBUG(log_) << "Sending <ASK>s.";
			encoder_produce(&ask);
		}
	}
	if (!ask.empty()) {
		ASSERT_NON_ZERO(log_, nhash);
		DEBUG(log_) << "Sending <ASK>s.";
		encoder_produce(&ask);
	} else {
		ASSERT_ZERO(log_, nhash);
	}

	return (true);
}

void
XCodecPipePair::encoder_consume(Buffer *buf)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT(log_, !encoder_sent_eos_);

	Buffer output;

	if (encoder_ == NULL) {
		Buffer extra;
		if (!codec_->cache()->uuid_encode(&extra)) {
			ERROR(log_) << "Could not encode UUID for <HELLO>.";
			encoder_error();
			return;
		}

		uint8_t len = extra.length();
		ASSERT(log_, len == UUID_SIZE);

		output.append(XCODEC_PIPE_OP_HELLO);
		output.append(len);
		output.append(extra);

		ASSERT(log_, output.length() == 2 + UUID_SIZE);

		encoder_ = new XCodecEncoder(codec_->cache());
	}

	if (!buf->empty()) {
		/*
		 * We must encode XCODEC_PIPE_MAX_FRAME / 2 bytes at a time,
		 * since at worst we double the size of data, and that way we
		 * can ensure that each frame is self-contained!
		 *
		 * Here we should also be doing protocol-aware framing.  Have
		 * a protocol subsystem which will taste if this is the first
		 * few frames, until it works out what framing policy to use.
		 * It could also decide to rewrite data in safe ways, rather
		 * than just do framing.
		 *
		 * XXX
		 * This needs to include a checksum of the decoded data, and
		 * we need a way to negotiate the resulting ASK/LEARN work, or
		 * even send the full decoded data in the raw in the case
		 * where an error is encountered.  That wouldn't be very hard
		 * to implement.
		 */
		for (;;) {
			uint32_t framelen;
			if (buf->length() <= XCODEC_PIPE_MAX_FRAME / 2)
				framelen = buf->length();
			else
				framelen = XCODEC_PIPE_MAX_FRAME / 2;

			Buffer frame;
			buf->moveout(&frame, framelen);

			std::map<uint64_t, BufferSegment *> *refmap =
				new std::map<uint64_t, BufferSegment *>;

			Buffer encoded;
			encoder_->encode(&encoded, &frame, refmap);
			ASSERT(log_, !encoded.empty());

			/*
			 * Track all references associated with this frame, so
			 * that we can guarantee we can answer any <ASK> for it
			 * until the peer has said they're finished with it.
			 */
			encoder_reference_frames_.push_back(refmap);

			/*
			 * Now wrap the encoded data frame.
			 */
			ASSERT(log_, encoded.length() <= XCODEC_PIPE_MAX_FRAME);

			framelen = encoded.length();
			framelen = BigEndian::encode(framelen);

			output.append(XCODEC_PIPE_OP_FRAME);
			output.append(&framelen);
			output.append(encoded);

			if (buf->empty())
				break;
		}
	} else {
		ASSERT(log_, !encoder_sent_eos_);
		output.append(XCODEC_PIPE_OP_EOS);
		encoder_sent_eos_ = true;
	}
	ASSERT(log_, !output.empty());
	encoder_produce(&output);
}
