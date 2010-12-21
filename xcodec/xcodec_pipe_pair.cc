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
 * And now for something completely different, a note on how end-of-stream
 * indication works with the XCodec.
 *
 * When things are going along smoothly, the XCodec is a nice one-way stream
 * compressor.  All you need is state that you already have or state from
 * earlier in the stream.  However, it doesn't take much for things to go less
 * smoothly.  When you have two connections, a symbol may be defined in the
 * first and referenced in the second, and the reference in the second stream
 * may be decoded before the definition in the first one.  In this case, we
 * have <ASK> and <LEARN> in the <OOB> stream to communicate bidirectionally
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

static void encode_frame(Buffer *, Buffer *);
static void encode_oob(Buffer *, Buffer *);

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
		if (decoder_buffer_.length() < sizeof (uint8_t) + sizeof (uint8_t) + sizeof (uint16_t))
			break;

		uint8_t magic;
		decoder_buffer_.extract(&magic);
		switch (magic) {
		case XCODEC_MAGIC:
			break;
		default:
			ERROR(log_) << "Expected magic and got another character.";
			decoder_error();
			return;
		}

		uint8_t op;
		decoder_buffer_.extract(&op, sizeof magic);
		switch (op) {
		case XCODEC_OP_FRAME:
			if (decoder_ == NULL) {
				ERROR(log_) << "Got frame data before decoder initialized.";
				decoder_error();
				return;
			}
			break;
		case XCODEC_OP_OOB:
			break;
		default:
			ERROR(log_) << "Got unframed data; remote codec must be out-of-date.";
			decoder_error();
			return;
		}

		uint16_t len;
		decoder_buffer_.extract(&len, sizeof magic + sizeof op);
		len = BigEndian::decode(len);
		if (len == 0 || len > XCODEC_FRAME_LENGTH) {
			ERROR(log_) << "Invalid framed data length.";
			decoder_error();
			return;
		}

		if (decoder_buffer_.length() < sizeof magic + sizeof op + sizeof len + len)
			return;

		Buffer data;
		decoder_buffer_.moveout(&data, sizeof magic + sizeof op + sizeof len, len);

		switch (op) {
		case XCODEC_OP_OOB:
			if (!decode_oob(&data)) {
				ERROR(log_) << "Error in OOB stream.";
				decoder_error();
				return;
			}
			break;
		case XCODEC_OP_FRAME:
			data.moveout(&decoder_frame_buffer_, data.length());
			break;
		default:
			NOTREACHED();
		}

		if (decoder_received_eos_ && !encoder_sent_eos_ack_) {
			DEBUG(log_) << "Decoder finished, got <EOS>, sending <EOS_ACK>.";

			Buffer eos_ack;
			eos_ack.append(XCODEC_MAGIC);
			eos_ack.append(XCODEC_OP_EOS_ACK);

			Buffer oob;
			encode_oob(&oob, &eos_ack);

			encoder_produce(&oob);
			encoder_sent_eos_ack_ = true;
		}

		if (decoder_frame_buffer_.empty())
			continue;

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
			 * we're waiting on the next frame.  It would be nice
			 * to make the encoder framing aware so that it would
			 * not end up with encoded data that straddles a frame
			 * boundary.  (Fixing that would also allow us to
			 * simplify length checking within the decoder
			 * considerably.)
			 */
			ASSERT(!decoder_frame_buffer_.empty());
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

bool
XCodecPipePair::decode_oob(Buffer *buf)
{
	while (!buf->empty()) {
		uint8_t magic = buf->peek();
		if (magic != XCODEC_MAGIC) {
			ERROR(log_) << "Expected magic and got another character in OOB stream.";
			return (false);
		}

		if (buf->empty()) {
			ERROR(log_) << "Missing operation in OOB stream.";
			return (false);
		}

		uint8_t op;
		buf->moveout(&op, sizeof magic, sizeof op);
		switch (op) {
		case XCODEC_OP_HELLO:
			if (decoder_cache_ != NULL) {
				ERROR(log_) << "Got <HELLO> twice.";
				return (false);
			} else {
				if (buf->length() < sizeof (uint8_t)) {
					ERROR(log_) << "Truncated <HELLO>.";
					return (false);
				}

				uint8_t len = buf->peek();
				buf->skip(1);

				if (buf->length() < len) {
					ERROR(log_) << "Truncated OOB stream.";
					return (false);
				}

				if (len != UUID_SIZE) {
					ERROR(log_) << "Unsupported <HELLO> length: " << (unsigned)len;
					return (false);
				}

				Buffer uubuf;
				buf->moveout(&uubuf, UUID_SIZE);

				UUID uuid;
				if (!uuid.decode(&uubuf)) {
					ERROR(log_) << "Invalid UUID in <HELLO>.";
					return (false);
				}

				decoder_cache_ = XCodecCache::lookup(uuid);
				ASSERT(decoder_cache_ != NULL);

				ASSERT(decoder_ == NULL);
				decoder_ = new XCodecDecoder(decoder_cache_);

				DEBUG(log_) << "Peer connected with UUID: " << uuid.string_;
			}
			break;
		case XCODEC_OP_ASK:
			if (encoder_ == NULL) {
				ERROR(log_) << "Got <ASK> before sending <HELLO>.";
				return (false);
			} else {
				uint64_t hash;
				buf->extract(&hash);
				buf->skip(sizeof hash);
				hash = BigEndian::decode(hash);

				BufferSegment *oseg = codec_->cache()->lookup(hash);
				if (oseg == NULL) {
					ERROR(log_) << "Unknown hash in <ASK>: " << hash;
					return (false);
				}

				DEBUG(log_) << "Responding to <ASK> with <LEARN>.";

				Buffer learn;
				learn.append(XCODEC_MAGIC);
				learn.append(XCODEC_OP_LEARN);
				learn.append(oseg);
				oseg->unref();

				Buffer oob;
				encode_oob(&oob, &learn);

				encoder_produce(&oob);
			}
			break;
		case XCODEC_OP_LEARN:
			if (decoder_cache_ == NULL) {
				ERROR(log_) << "Got <LEARN> before <HELLO>.";
				return (false);
			} else {
				BufferSegment *seg;
				buf->copyout(&seg, XCODEC_SEGMENT_LENGTH);
				buf->skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash<XCODEC_SEGMENT_LENGTH>::hash(seg->data());
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
		case XCODEC_OP_EOS:
			if (decoder_received_eos_) {
				ERROR(log_) << "Duplicate <EOS>.";
				return (false);
			}
			decoder_received_eos_ = true;
			break;
		case XCODEC_OP_EOS_ACK:
			if (!encoder_sent_eos_) {
				ERROR(log_) << "Got <EOS_ACK> before sending <EOS>.";
				return (false);
			}
			if (decoder_received_eos_ack_) {
				ERROR(log_) << "Duplicate <EOS_ACK>.";
				return (false);
			}
			decoder_received_eos_ack_ = true;
			break;
		default:
			ERROR(log_) << "Unsupported operation in OOB stream.";
			return (false);
		}
	}

	return (true);
}

void
XCodecPipePair::encoder_consume(Buffer *buf)
{
	ASSERT(!encoder_sent_eos_);

	Buffer output;

	if (encoder_ == NULL) {
		if (buf->empty()) {
			INFO(log_) << "Encoder received EOS before any data.";
			encoder_produce(buf);
			return;
		}

		Buffer extra;
		if (!codec_->cache()->uuid_encode(&extra)) {
			ERROR(log_) << "Could not encode UUID for <HELLO>.";
			encoder_error();
			return;
		}

		uint8_t len = extra.length();
		ASSERT(len == UUID_SIZE);

		Buffer hello;
		hello.append(XCODEC_MAGIC);
		hello.append(XCODEC_OP_HELLO);
		hello.append(len);
		hello.append(extra);

		ASSERT(hello.length() == 3 + UUID_SIZE);

		encode_oob(&output, &hello);

		encoder_ = new XCodecEncoder(codec_->cache());
	}

	if (!buf->empty()) {
		Buffer encoded;
		encoder_->encode(&encoded, buf);
		ASSERT(!encoded.empty());

		encode_frame(&output, &encoded);
		ASSERT(!output.empty());

		encoder_produce(&output);
	} else {
		Buffer eos;
		eos.append(XCODEC_MAGIC);
		eos.append(XCODEC_OP_EOS);

		encode_oob(&output, &eos);

		encoder_produce(&output);

		encoder_sent_eos_ = true;
	}
}

static void
encode_frame(Buffer *out, Buffer *in)
{
	while (!in->empty()) {
		uint16_t framelen;
		if (in->length() <= XCODEC_FRAME_LENGTH)
			framelen = in->length();
		else
			framelen = XCODEC_FRAME_LENGTH;

		Buffer frame;
		in->moveout(&frame, framelen);

		framelen = BigEndian::encode(framelen);

		out->append(XCODEC_MAGIC);
		out->append(XCODEC_OP_FRAME);
		out->append(&framelen);
		out->append(frame);
	}
}

static void
encode_oob(Buffer *out, Buffer *in)
{
	ASSERT(!in->empty());
	ASSERT(in->length() <= XCODEC_FRAME_LENGTH);

	uint16_t ooblen = in->length();
	ooblen = BigEndian::encode(ooblen);

	out->append(XCODEC_MAGIC);
	out->append(XCODEC_OP_OOB);
	out->append(&ooblen);
	in->moveout(out, in->length());
}
