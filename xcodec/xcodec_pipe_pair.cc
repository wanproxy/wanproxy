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

/*
 * XXX
 * This is no longer up-to-date.
 *
 * And now for something completely different, a note on how end-of-stream
 * indication works with the XCodec.
 *
 * When things are going along smoothly, the XCodec is a nice one-way stream
 * compressor.  All you need is state that you already have or state from
 * earlier in the stream.  However, it doesn't take much for things to go less
 * smoothly.  When you have two connections, a symbol may be defined in the
 * first and referenced in the second, and the reference in the second stream
 * may be decoded before the definition in the first one.  In this case, we
 * have <ASK> and <LEARN> in the stream to communicate bidirectionally
 * to get the reference.  If we're actually going to get the definition soon,
 * that's a bit wasteful, and there are a lot of optimizations we can make,
 * but the basic principle needs to be robust in case, say, the first
 * connection goes down.
 *
 * Because of this, we can't just pass through end-of-stream indicators
 * freely.  When the encoder receives EOS from a StreamChannel, we could then
 * send EOS out to the StreamChannel that connects us to the decoder on the
 * other side of the network.  But what if that decoder needs to <ASK> us
 * about a symbol we sent a reference to just before EOS?
 *
 * So we send <EOS> rather than EOS, a message saying that the encoded stream
 * has ended.
 *
 * When the decoder receives <EOS> it can send EOS on to the StreamChannel it
 * is writing to, assuming it has processed all outstanding frame data.  And
 * when it has finished processing all outstanding frame data, it will send
 * <EOS_ACK> on the encoder's output StreamChannel, to the remote decoder.
 * When both sides have sent <EOS_ACK>, the encoder's StreamChannels may be
 * shut down and no more communication will occur.
 */

/*
 * Usage:
 * 	<OP_HELLO> length[uint8_t] data[uint8_t x length]
 *
 * Effects:
 * 	Must appear at the start of and only at the start of an encoded	stream.
 *
 * Sife-effects:
 * 	Possibly many.
 */
#define	XCODEC_PIPE_OP_HELLO	((uint8_t)0xff)

/*
 * Usage:
 * 	<OP_LEARN> data[uint8_t x XCODEC_PIPE_SEGMENT_LENGTH]
 *
 * Effects:
 * 	The `data' is hashed, the hash is associated with the data if possible.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_LEARN	((uint8_t)0xfe)

/*
 * Usage:
 * 	<OP_ASK> hash[uint64_t]
 *
 * Effects:
 * 	An OP_LEARN will be sent in response with the data corresponding to the
 * 	hash.
 *
 * 	If the hash is unknown, error will be indicated.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_ASK	((uint8_t)0xfd)

/*
 * Usage:
 * 	<OP_EOS>
 *
 * Effects:
 * 	Alert the other party that we have no intention of sending more data.
 *
 * Side-effects:
 * 	The other party will send <OP_EOS_ACK> when it has processed all of
 * 	the data we have sent.
 */
#define	XCODEC_PIPE_OP_EOS	((uint8_t)0xfc)

/*
 * Usage:
 * 	<OP_EOS_ACK>
 *
 * Effects:
 * 	Alert the other party that we have no intention of reading more data.
 *
 * Side-effects:
 * 	The connection will be torn down.
 */
#define	XCODEC_PIPE_OP_EOS_ACK	((uint8_t)0xfb)

/*
 * Usage:
 * 	<OP_FRAME> length[uint32_t] data[uint8_t x length]
 *
 * Effects:
 * 	Frames an encoded chunk.
 *
 * Side-effects:
 * 	The other party will send <OP_ADVANCE> when it has processed one or
 * 	more frames successfully in their entirety.
 */
#define	XCODEC_PIPE_OP_FRAME	((uint8_t)0x02)

/*
 * Usage:
 * 	<OP_ADVANCE> count[uint32_t]
 *
 * Effects:
 * 	The other party is informed that we have processed the last `count'
 * 	frames, and that it need not retain segments referenced within them
 * 	in case an ASK is generated by the encoder.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_ADVANCE	((uint8_t)0x01)

/*
 * Allow up to 1MB of data per encoded frame.
 */
#define	XCODEC_PIPE_MAX_FRAME	(1024 * 1024)

void
XCodecPipePair::decoder_consume(Buffer *buf)
{
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
	 * state.
	 */
	if (!decoder_decode_data()) {
		decoder_error();
		encoder_pipe_->uncork();
		return;
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

				decoder_cache_ = XCodecCache::lookup(uuid);
				if (decoder_cache_ == NULL) {
					decoder_cache_ = new XCodecMemoryCache(uuid);
					XCodecCache::enter(uuid, decoder_cache_);
				}

				ASSERT(log_, decoder_ == NULL);
				decoder_ = new XCodecDecoder(decoder_cache_);

				DEBUG(log_) << "Peer connected with UUID: " << uuid.string_;
			}
			break;
		case XCODEC_PIPE_OP_ASK:
			if (encoder_ == NULL) {
				ERROR(log_) << "Got <ASK> before sending <HELLO>.";
				return (false);
			} else {
				uint64_t hash;
				if (decoder_buffer_.length() < sizeof op + sizeof hash)
					return (true);

				decoder_buffer_.skip(sizeof op);

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

					DEBUG(log_) << "Responding to <ASK> with <LEARN>.";

					Buffer learn;
					learn.append(XCODEC_PIPE_OP_LEARN);
					learn.append(it->second);

					encoder_produce(&learn);
					break;
				}
				if (rmit == encoder_reference_frames_.end()) {
					ERROR(log_) << "Hash in <ASK> could not be found in any reference frame: "<< hash;
					return (false);
				}
			}
			break;
		case XCODEC_PIPE_OP_LEARN:
			if (decoder_cache_ == NULL) {
				ERROR(log_) << "Got <LEARN> before <HELLO>.";
				return (false);
			} else {
				if (decoder_buffer_.length() < sizeof op + XCODEC_SEGMENT_LENGTH)
					return (true);

				decoder_buffer_.skip(sizeof op);

				BufferSegment *seg;
				decoder_buffer_.copyout(&seg, XCODEC_SEGMENT_LENGTH);
				decoder_buffer_.skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash::hash(seg->data());
				if (decoder_unknown_hashes_.find(hash) == decoder_unknown_hashes_.end()) {
					INFO(log_) << "Gratuitous <LEARN> without <ASK>.";
				} else {
					decoder_unknown_hashes_.erase(hash);
				}

				BufferSegment *oseg = decoder_cache_->lookup(hash);
				if (oseg != NULL) {
					if (!oseg->equal(seg)) {
						oseg->unref();
						ERROR(log_) << "Collision in <LEARN>.";
						seg->unref();
						return (false);
					}
					oseg->unref();
					DEBUG(log_) << "Redundant <LEARN>.";
				} else {
					DEBUG(log_) << "Successful <LEARN>.";
					decoder_cache_->enter(hash, seg);
				}
				seg->unref();
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

	if (!decoder_unknown_hashes_.empty()) {
		DEBUG(log_) << "Waiting for unknown hashes to continue processing data.";
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

	Buffer ask;
	std::set<uint64_t>::const_iterator it;
	for (it = decoder_unknown_hashes_.begin(); it != decoder_unknown_hashes_.end(); ++it) {
		uint64_t hash = *it;
		hash = BigEndian::encode(hash);

		ask.append(XCODEC_PIPE_OP_ASK);
		ask.append(&hash);
	}
	if (!ask.empty()) {
		DEBUG(log_) << "Sending <ASK>s.";
		encoder_produce(&ask);
	}

	return (true);
}

void
XCodecPipePair::encoder_consume(Buffer *buf)
{
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
