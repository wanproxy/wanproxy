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
 * 	<FRAME> length[uint16_t] data[uint8_t x length]
 *
 * Effects:
 * 	Frames an encoded chunk.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_PIPE_OP_FRAME	((uint8_t)0x00)

#define	XCODEC_PIPE_MAX_FRAME	(32768)

static void encode_frame(Buffer *, Buffer *);

void
XCodecPipePair::decoder_consume(Buffer *buf)
{
	if (buf->empty()) {
		if (!decoder_buffer_.empty())
			ERROR(log_) << "Remote encoder closed connection with data outstanding.";
		if (!decoder_frame_buffer_.empty())
			ERROR(log_) << "Remote encoder closed connection with frame data outstanding.";
		if (!decoder_sent_eos_) {
			decoder_sent_eos_ = true;
			decoder_produce(buf);
		}
		return;
	}

	decoder_buffer_.append(buf);
	buf->clear();

	while (!decoder_buffer_.empty()) {
		uint8_t op = decoder_buffer_.peek();
		switch (op) {
		case XCODEC_PIPE_OP_HELLO:
			if (decoder_cache_ != NULL) {
				ERROR(log_) << "Got <HELLO> twice.";
				decoder_error();
				return;
			} else {
				uint8_t len;
				if (decoder_buffer_.length() < sizeof op + sizeof len)
					return;
				decoder_buffer_.extract(&len, sizeof op);

				if (decoder_buffer_.length() < sizeof op + sizeof len + len)
					return;

				if (len != UUID_SIZE) {
					ERROR(log_) << "Unsupported <HELLO> length: " << (unsigned)len;
					decoder_error();
					return;
				}

				Buffer uubuf;
				decoder_buffer_.moveout(&uubuf, sizeof op + sizeof len, UUID_SIZE);

				UUID uuid;
				if (!uuid.decode(&uubuf)) {
					ERROR(log_) << "Invalid UUID in <HELLO>.";
					decoder_error();
					return;
				}

				decoder_cache_ = XCodecCache::lookup(uuid);
				ASSERT(decoder_cache_ != NULL);

				ASSERT(decoder_ == NULL);
				decoder_ = new XCodecDecoder(decoder_cache_);

				DEBUG(log_) << "Peer connected with UUID: " << uuid.string_;
			}
			break;
		case XCODEC_PIPE_OP_ASK:
			if (encoder_ == NULL) {
				ERROR(log_) << "Got <ASK> before sending <HELLO>.";
				decoder_error();
				return;
			} else {
				uint64_t hash;
				if (decoder_buffer_.length() < sizeof op + sizeof hash)
					return;

				decoder_buffer_.skip(sizeof op);

				decoder_buffer_.extract(&hash);
				decoder_buffer_.skip(sizeof hash);
				hash = BigEndian::decode(hash);

				BufferSegment *oseg = codec_->cache()->lookup(hash);
				if (oseg == NULL) {
					ERROR(log_) << "Unknown hash in <ASK>: " << hash;
					decoder_error();
					return;
				}

				DEBUG(log_) << "Responding to <ASK> with <LEARN>.";

				Buffer learn;
				learn.append(XCODEC_PIPE_OP_LEARN);
				learn.append(oseg);
				oseg->unref();

				encoder_produce(&learn);
			}
			break;
		case XCODEC_PIPE_OP_LEARN:
			if (decoder_cache_ == NULL) {
				ERROR(log_) << "Got <LEARN> before <HELLO>.";
				decoder_error();
				return;
			} else {
				if (decoder_buffer_.length() < sizeof op + XCODEC_SEGMENT_LENGTH)
					return;

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
						decoder_error();
						return;
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
				decoder_error();
				return;
			}
			decoder_buffer_.skip(1);
			decoder_received_eos_ = true;
			break;
		case XCODEC_PIPE_OP_EOS_ACK:
			if (!encoder_sent_eos_) {
				ERROR(log_) << "Got <EOS_ACK> before sending <EOS>.";
				decoder_error();
				return;
			}
			if (decoder_received_eos_ack_) {
				ERROR(log_) << "Duplicate <EOS_ACK>.";
				decoder_error();
				return;
			}
			decoder_buffer_.skip(1);
			decoder_received_eos_ack_ = true;
			break;
		case XCODEC_PIPE_OP_FRAME:
			if (decoder_ == NULL) {
				ERROR(log_) << "Got frame data before decoder initialized.";
				decoder_error();
				return;
			} else {
				uint16_t len;
				if (decoder_buffer_.length() < sizeof op + sizeof len)
					return;
				decoder_buffer_.extract(&len, sizeof op);
				len = BigEndian::decode(len);
				if (len == 0 || len > XCODEC_PIPE_MAX_FRAME) {
					ERROR(log_) << "Invalid framed data length.";
					decoder_error();
					return;
				}

				if (decoder_buffer_.length() < sizeof op + sizeof len + len)
					return;

				decoder_buffer_.moveout(&decoder_frame_buffer_, sizeof op + sizeof len, len);
			}
			break;
		default:
			ERROR(log_) << "Unsupported operation in pipe stream.";
			decoder_error();
			return;
		}

		if (decoder_frame_buffer_.empty()) {
			if (decoder_received_eos_ && !encoder_sent_eos_ack_) {
				DEBUG(log_) << "Decoder finished, got <EOS>, sending <EOS_ACK>.";

				Buffer eos_ack;
				eos_ack.append(XCODEC_PIPE_OP_EOS_ACK);

				encoder_produce(&eos_ack);
				encoder_sent_eos_ack_ = true;
			}
			continue;
		}

		if (!decoder_unknown_hashes_.empty()) {
			DEBUG(log_) << "Waiting for unknown hashes to continue processing data.";
			continue;
		}

		Buffer output;
		if (!decoder_->decode(&output, &decoder_frame_buffer_, decoder_unknown_hashes_)) {
			ERROR(log_) << "Decoder exiting with error.";
			decoder_error();
			return;
		}

		if (!output.empty()) {
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
			ASSERT(!decoder_frame_buffer_.empty() || !decoder_unknown_hashes_.empty());
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
	}

	if (decoder_received_eos_ && !decoder_sent_eos_) {
		DEBUG(log_) << "Decoder finished, got <EOS>, shutting down decoder output channel.";
		Buffer eos;
		decoder_produce(&eos);
		decoder_sent_eos_ = true;
	}

	if (encoder_sent_eos_ack_ && decoder_received_eos_ack_) {
		ASSERT(decoder_buffer_.empty());
		ASSERT(decoder_frame_buffer_.empty());

		DEBUG(log_) << "Decoder finished, got <EOS_ACK>, shutting down encoder output channel.";

		Buffer eos;
		encoder_produce(&eos);
	}
}

void
XCodecPipePair::encoder_consume(Buffer *buf)
{
	ASSERT(!encoder_sent_eos_);

	Buffer output;

	if (encoder_ == NULL) {
		Buffer extra;
		if (!codec_->cache()->uuid_encode(&extra)) {
			ERROR(log_) << "Could not encode UUID for <HELLO>.";
			encoder_error();
			return;
		}

		uint8_t len = extra.length();
		ASSERT(len == UUID_SIZE);

		output.append(XCODEC_PIPE_OP_HELLO);
		output.append(len);
		output.append(extra);

		ASSERT(output.length() == 2 + UUID_SIZE);

		encoder_ = new XCodecEncoder(codec_->cache());
	}

	if (!buf->empty()) {
		Buffer encoded;
		encoder_->encode(&encoded, buf);
		ASSERT(!encoded.empty());

		encode_frame(&output, &encoded);
		ASSERT(!output.empty());
	} else {
		output.append(XCODEC_PIPE_OP_EOS);
		encoder_sent_eos_ = true;
	}
	encoder_produce(&output);
}

static void
encode_frame(Buffer *out, Buffer *in)
{
	while (!in->empty()) {
		uint16_t framelen;
		if (in->length() <= XCODEC_PIPE_MAX_FRAME)
			framelen = in->length();
		else
			framelen = XCODEC_PIPE_MAX_FRAME;

		Buffer frame;
		in->moveout(&frame, framelen);

		framelen = BigEndian::encode(framelen);

		out->append(XCODEC_PIPE_OP_FRAME);
		out->append(&framelen);
		out->append(frame);
	}
}
