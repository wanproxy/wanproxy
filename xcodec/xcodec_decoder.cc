#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>

XCodecDecoder::XCodecDecoder(XCodecCache *cache)
: log_("/xcodec/decoder"),
  cache_(cache),
  window_()
{ }

XCodecDecoder::~XCodecDecoder()
{ }

/*
 * XXX These comments are out-of-date.
 *
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
XCodecDecoder::decode(Buffer *output, Buffer *input, std::set<uint64_t>& unknown_hashes)
{
	while (!input->empty()) {
		unsigned off;
		if (!input->find(XCODEC_MAGIC, &off)) {
			output->append(input);
			input->clear();

			break;
		}

		if (off != 0) {
			output->append(input, off);
			input->skip(off);
		}
		ASSERT(!input->empty());

		/*
		 * Need the following byte at least.
		 */
		if (input->length() == 1)
			break;

		uint8_t op;
		input->copyout(&op, sizeof XCODEC_MAGIC, sizeof op);

		switch (op) {
		case XCODEC_OP_ESCAPE:
			output->append(XCODEC_MAGIC);
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

				uint64_t hash = XCodecHash::hash(seg->data());
				BufferSegment *oseg = cache_->lookup(hash);
				if (oseg != NULL) {
					if (oseg->equal(seg)) {
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

				window_.declare(hash, seg);
				output->append(seg);
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
					if (unknown_hashes.find(hash) == unknown_hashes.end()) {
						DEBUG(log_) << "Sending <ASK>, waiting for <LEARN>.";
						unknown_hashes.insert(hash);
					} else {
						DEBUG(log_) << "Already sent <ASK>, waiting for <LEARN>.";
					}

					return (true);
				} else {
					window_.declare(hash, oseg);
					output->append(oseg);
					oseg->unref();
				}
			}
			break;
		case XCODEC_OP_BACKREF:
			if (input->length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t))
				goto done;
			else {
				uint8_t idx;
				input->moveout(&idx, sizeof XCODEC_MAGIC + sizeof op, sizeof idx);

				BufferSegment *oseg = window_.dereference(idx);
				if (oseg == NULL) {
					ERROR(log_) << "Index not present in <BACKREF> window: " << (unsigned)idx;
					return (false);
				}

				output->append(oseg);
				oseg->unref();
			}
			break;
		default:
			ERROR(log_) << "Unsupported XCodec opcode " << (unsigned)op << ".";
			return (false);
		}
	}
done:	return (true);
}
