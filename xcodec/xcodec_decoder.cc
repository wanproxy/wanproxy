#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcdb.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>

XCodecDecoder::XCodecDecoder(XCodec *codec)
: log_("/xcodec/decoder"),
  database_(codec->database_),
  backref_()
{ }

XCodecDecoder::~XCodecDecoder()
{ }

bool
XCodecDecoder::decode(Buffer *output, Buffer *input)
{
	BufferSegment *oseg, *seg;
	uint64_t sum;

	while (!input->empty()) {
		switch (input->peek()) {
		case XCODEC_HASHREF_CHAR:
			if (input->length() - 1 < sizeof sum) {
				DEBUG(log_) << "Short HASHREF.";
				return (true);
			}

			input->moveout((uint8_t *)&sum, 1, sizeof sum);
			sum = LittleEndian::decode(sum);
			seg = database_->lookup(sum);
			if (seg == NULL) {
				ERROR(log_ + "/decode") << "hash (" << sum << ") not in database.";
				return (false);
			}
			backref_.declare(sum, seg);
			output->append(seg);
			seg->unref();
			break;
		case XCODEC_ESCAPE_CHAR:
			if (input->length() - 1 < 1) {
				DEBUG(log_) << "Short ESCAPE.";
				return (true);
			}
			input->skip(1);
			output->append(input->peek());
			input->skip(1);
			break;
		case XCODEC_DECLARE_CHAR:
			if (input->length() - 1 < sizeof sum + XCODEC_CHUNK_LENGTH) {
				DEBUG(log_) << "Short DECLARE.";
				return (true);
			}

			input->moveout((uint8_t *)&sum, 1, sizeof sum);
			sum = LittleEndian::decode(sum);
			input->copyout(&seg, XCODEC_CHUNK_LENGTH);
			input->skip(XCODEC_CHUNK_LENGTH);

			/*
			 * Make sure this checksum is correct.
			 */
			ASSERT(XCHash<XCODEC_CHUNK_LENGTH>::hash(seg->data()) == sum);

			oseg = database_->lookup(sum);
			if (oseg != NULL) {
				/*
				 * If this hash is already in the database for
				 * this data, use that BufferSegment.
				 */
				if (oseg->match(seg)) {
					seg->unref();
					seg = oseg;
					break;
				}
				oseg->unref();

				ERROR(log_ + "/decode") << "hash (" << sum << ") shadows database definition.";
				/*
				 * XXX
				 * Override the database entry.
				 */
				return (false);
			} else {
				database_->enter(sum, seg);
				/*
				 * XXX
				 * We have no collision, enter this
				 * into the database, as well.
				 */
			}

			backref_.declare(sum, seg);
			seg->unref();
			break;
		case XCODEC_BACKREF_CHAR:
			/*
			 * We expect long strings of BACKREF since we
			 * use BACKREFs after our DECLAREs.  So use a
			 * loop here.
			 */
			while (input->peek() == XCODEC_BACKREF_CHAR) {
				if (input->length() - 1 < 1) {
					DEBUG(log_) << "Short BACKREF.";
					return (true);
				}

				uint8_t b;
				input->moveout(&b, 1, sizeof b);

				seg = backref_.dereference(b);
				ASSERT(seg != NULL);
				output->append(seg);
				seg->unref();

				if (input->empty())
					break;
			}
			break;
		default:
			output->append(input->peek());
			input->skip(1);
			break;
		}
	}
	return (true);
}
