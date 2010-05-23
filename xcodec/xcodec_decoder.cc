#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>

XCodecDecoder::XCodecDecoder(void)
: log_("/xcodec/decoder"),
  cache_(NULL),
  window_(),
  encoder_(NULL),
  queued_(),
  asked_()
{ }

XCodecDecoder::~XCodecDecoder()
{
	ASSERT(encoder_ == NULL);
	if (!asked_.empty())
		DEBUG(log_) << asked_.size() << " outstanding <ASK>s when destroyed.";
}

/*
 * Decode an XCodec-encoded stream.  Returns false if there was an
 * inconsistency, error or unrecoverable condition in the stream.
 * Returns true if we were able to process the stream entirely or
 * expect to be able to finish processing it once more data arrives.
 * The input buffer is cleared of anything we can parse right now.
 *
 * Since some events later in the stream (i.e. ASK or LEARN) may need
 * to be processed before some earlier in the stream (i.e. REF), we
 * parse the stream into a list of actions to take, performing them
 * as we go if possible, otherwise queueing them to occur until the
 * action that is blocking the stream has been satisfied or the stream
 * has been closed.
 *
 * XXX For now we will ASK in every stream where an unknown hash has
 * occurred and expect a LEARN in all of them.  In the future, it is
 * desirable to optimize this.  Especially once we start putting an
 * instance UUID in the HELLO message and can tell which streams
 * share an originator.
 */
bool
XCodecDecoder::decode(Buffer *output, Buffer *input)
{
	bool queued_data = false;

	for (;;) {
		/*
		 * If there is queued data and no more outstanding hashes,
		 * handle the queued data.
		 */
		if (!queued_.empty() && asked_.empty()) {
			DEBUG(log_) << "Handling queued data.";
			/* XXX input->prepend(queued_);  */

			Buffer tmp(queued_);
			queued_.clear();
			tmp.append(input);
			input->clear();
			input->append(tmp);
		}

		if (input->empty())
			break;

		unsigned off;
		if (!input->find(XCODEC_MAGIC, &off)) {
			if (queued_.empty()) {
				output->append(input);
			} else {
				queued_.append(input);
			}

			input->clear();

			break;
		}

		if (off != 0) {
			if (queued_.empty()) {
				output->append(input, off);
				input->skip(off);
			} else {
				queued_.append(input, off);
				input->skip(off);
			}
		}
		ASSERT(!input->empty());

		/*
		 * Need the following byte at least.
		 */
		if (input->length() == 1)
			break;

		uint8_t op;
		input->copyout(&op, sizeof XCODEC_MAGIC, sizeof op);

		if (cache_ == NULL && op != XCODEC_OP_HELLO) {
			ERROR(log_) << "No cache selected before <HELLO>.";
			return (false);
		}

		switch (op) {
		case XCODEC_OP_HELLO:
			if (cache_ != NULL) {
				ERROR(log_) << "Got <HELLO> twice.";
				return (false);
			}

			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t) + UUID_SIZE)
				goto done;
			else {
				if (!queued_.empty()) {
					ERROR(log_) << "Got <HELLO> with data queued.";
					return (false);
				}

				uint8_t len;
				input->copyout(&len, sizeof XCODEC_MAGIC + sizeof op, sizeof len);
				if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof len + len)
					goto done;
				UUID uuid;
				switch (len) {
				case 0:
					/* XXX Compatability?  */
					uuid.generate();
					break;
				case UUID_SIZE: {
					Buffer uubuf;
					uubuf.append(input, sizeof XCODEC_MAGIC + sizeof op + sizeof len + len);
					uubuf.skip(sizeof XCODEC_MAGIC + sizeof op + sizeof len);
					ASSERT(uubuf.length() == UUID_SIZE);
					if (!uuid.decode(&uubuf)) {
						ERROR(log_) << "Invalid UUID in <HELLO>.";
						return (false);
					}

					break;
				}
				default:
					ERROR(log_) << "Unsupported <HELLO> length: " << (unsigned)len;
					return (false);
				}
				input->skip(sizeof XCODEC_MAGIC + sizeof op + sizeof len + len);

				cache_ = XCodecCache::lookup(uuid);
				ASSERT(cache_ != NULL);

				INFO(log_) << "Peer connected with UUID: " << uuid.string_;
			}
			break;
		case XCODEC_OP_ESCAPE:
			if (queued_.empty()) {
				output->append(XCODEC_MAGIC);
			} else {
				queued_.append(XCODEC_MAGIC);
				queued_.append(XCODEC_OP_ESCAPE);
			}
			input->skip(sizeof XCODEC_MAGIC + sizeof op);
			break;
		case XCODEC_OP_EXTRACT:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + XCODEC_SEGMENT_LENGTH)
				goto done;
			else {
				input->skip(sizeof XCODEC_MAGIC + sizeof op);

				BufferSegment *seg;
				input->copyout(&seg, XCODEC_SEGMENT_LENGTH);
				input->skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash<XCODEC_SEGMENT_LENGTH>::hash(seg->data());
				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg != NULL) {
					if (oseg->match(seg)) {
						seg->unref();
						seg = oseg;
					} else {
						ERROR(log_) << "Collision in <EXTRACT>.";
						seg->unref();
						return (false);
					}
				} else {
					cache_->enter(hash, seg);
				}

				/* Just in case.  */
				asked_.erase(hash);

				if (queued_.empty()) {
					window_.declare(hash, seg);
					output->append(seg);
				} else {
					queued_.append(XCODEC_MAGIC);
					queued_.append(XCODEC_OP_EXTRACT);
					queued_.append(seg);
				}
				seg->unref();
			}
			break;
		case XCODEC_OP_REF:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint64_t))
				goto done;
			else {
				uint64_t behash;
				input->moveout((uint8_t *)&behash, sizeof XCODEC_MAGIC + sizeof op, sizeof behash);
				uint64_t hash = BigEndian::decode(behash);

				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg == NULL) {
					if (encoder_ == NULL) {
						ERROR(log_) << "Unknown hash in <REF>: " << hash;
						return (false);
					}

					if (asked_.find(hash) == asked_.end()) {
						DEBUG(log_) << "Sending <ASK>, waiting for <LEARN>.";
						encoder_->encode_ask(hash);
						asked_.insert(hash);

						queued_data = true;
					} else {
						DEBUG(log_) << "Already sent <ASK>, waiting for <LEARN>.";
					}

					queued_.append(XCODEC_MAGIC);
					queued_.append(XCODEC_OP_REF);
					queued_.append((const uint8_t *)&behash, sizeof behash);
				} else {
					if (queued_.empty()) {
						window_.declare(hash, oseg);
						output->append(oseg);
					} else {
						queued_.append(XCODEC_MAGIC);
						queued_.append(XCODEC_OP_REF);
						queued_.append((const uint8_t *)&behash, sizeof behash);
					}
					oseg->unref();
				}
			}
			break;
		case XCODEC_OP_BACKREF:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t))
				goto done;
			else {
				if (queued_.empty()) {
					uint8_t idx;
					input->moveout(&idx, sizeof XCODEC_MAGIC + sizeof op, sizeof idx);

					BufferSegment *oseg = window_.dereference(idx);
					if (oseg == NULL) {
						ERROR(log_) << "Index not present in <BACKREF> window: " << (unsigned)idx;
						return (false);
					}

					output->append(oseg);
					oseg->unref();
				} else {
					input->moveout(&queued_, sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t));
				}
			}
			break;
		case XCODEC_OP_LEARN:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + XCODEC_SEGMENT_LENGTH)
				goto done;
			else {
				input->skip(sizeof XCODEC_MAGIC + sizeof op);

				BufferSegment *seg;
				input->copyout(&seg, XCODEC_SEGMENT_LENGTH);
				input->skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash<XCODEC_SEGMENT_LENGTH>::hash(seg->data());
				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg != NULL) {
					if (!oseg->match(seg)) {
						oseg->unref();
						ERROR(log_) << "Collision in <LEARN>.";
						seg->unref();
						return (false);
					}
					oseg->unref();
					DEBUG(log_) << "Redundant <LEARN>.";
				} else {
					DEBUG(log_) << "Successful <LEARN>.";
					cache_->enter(hash, seg);
				}
				asked_.erase(hash);
				if (asked_.empty())
					DEBUG(log_) << "No outstanding <ASK>s after <LEARN>.";
				seg->unref();
			}
			break;
		case XCODEC_OP_ASK:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint64_t))
				goto done;
			else {
				if (encoder_ == NULL) {
					ERROR(log_) << "Cannot handle <ASK> without associated encoder.";
					return (false);
				}

				uint64_t behash;
				input->moveout((uint8_t *)&behash, sizeof XCODEC_MAGIC + sizeof op, sizeof behash);
				uint64_t hash = BigEndian::decode(behash);

				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg == NULL) {
					ERROR(log_) << "Unknown hash in <ASK>: " << hash;
					return (false);
				}

				DEBUG(log_) << "Responding to <ASK> with <LEARN>.";
				encoder_->encode_learn(oseg);
				oseg->unref();

				queued_data = true;
			}
			break;
		case XCODEC_OP_EOS:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op)
				goto done;
			else {
				input->skip(sizeof XCODEC_MAGIC + sizeof op);
				DEBUG(log_) << "Received <EOS>.  Bytes remaining: " << input->length();

				if (encoder_ != NULL) {
					encoder_->received_eos();
				}
			}
			break;
		default:
			ERROR(log_) << "Unsupported XCodec opcode " << (unsigned)op << ".";
			return (false);
		}
	}
done:	if (queued_data) {
		ASSERT(encoder_ != NULL);
		encoder_->encode_push();
	}
	return (true);
}

void
XCodecDecoder::set_encoder(XCodecEncoder *encoder)
{
	ASSERT(encoder != NULL || encoder_ != NULL);
	ASSERT(encoder_ == NULL || encoder == NULL);

	encoder_ = encoder;
}
