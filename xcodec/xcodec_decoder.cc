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
XCodecDecoder::reference(Buffer *output, Buffer *input)
{
	BufferSegment *seg;
	uint64_t sum;

	if (input->length() - 1 < sizeof sum)
		return (false);
	input->moveout((uint8_t *)&sum, 1, sizeof sum);
	sum = LittleEndian::decode(sum);
	seg = database_->lookup(sum);
	if (seg == NULL) {
		ERROR(log_ + "/decode") << "hash (" << sum << ") not in database.";
		throw sum;
	}
	backref_.declare(sum, seg);
	output->append(seg);
	seg->unref();
	return (true);
}

bool
XCodecDecoder::unescape(Buffer *output, Buffer *input)
{
	if (input->length() - 1 < 1)
		return (false);
	input->skip(1);
	output->append(input->peek());
	input->skip(1);
	return (true);
}

bool
XCodecDecoder::declare(Buffer *input)
{
	uint64_t sum;

	if (input->length() - 1 < sizeof sum + XCODEC_CHUNK_LENGTH)
		return (false);
	input->moveout((uint8_t *)&sum, 1, sizeof sum);
	sum = LittleEndian::decode(sum);
	BufferSegment *seg;
	input->copyout(&seg, XCODEC_CHUNK_LENGTH);
	input->skip(XCODEC_CHUNK_LENGTH);

	/*
	 * Make sure this checksum is correct.
	 */
	ASSERT(XCHash<XCODEC_CHUNK_LENGTH>::hash(seg->data()) == sum);

	backref_.declare(sum, seg);

	/*
	 * Duplicate hash - we may be decodeing a
	 * streaming mode file and using a persistent
	 * database.
	 *
	 * Local hash usage should override the
	 * database, but if the two are identical we
	 * have nothing to do.
	 */
	BufferSegment *old;

	old = database_->lookup(sum);
	if (old != NULL) {
		if (old->match(seg)) {
			old->unref();
			seg->unref();
			return (true);
		}
		old->unref();

		ERROR(log_ + "/decode") << "hash (" << sum << ") shadows database definition.";
		throw sum;
		/*
		 * XXX
		 * Override the database entry.
		 */
	} else {
		database_->enter(sum, seg);
		seg->unref();
		/*
		 * XXX
		 * We have no collision, enter this
		 * into the database, as well.
		 */
	}
	return (true);
}

bool
XCodecDecoder::backreference(Buffer *output, Buffer *input)
{
	BufferSegment *seg;

	while (input->peek() == XCODEC_BACKREF_CHAR) {
		if (input->length() - 1 < 1)
			return (false);
		input->skip(1);
		uint8_t b;
		b = input->peek();
		input->skip(1);
		seg = backref_.dereference(b);
		ASSERT(seg != NULL);
		output->append(seg);
		seg->unref();
		if (input->empty())
			break;
	}
	return (true);
}

bool
XCodecDecoder::decode(Buffer *output, Buffer *input)
{
	while (!input->empty()) {
		switch (input->peek()) {
		case XCODEC_HASHREF_CHAR:
			try {
				if (!reference(output, input))
					return (true);
			} catch (uint64_t sum) {
				return (false);
			}
			break;
		case XCODEC_ESCAPE_CHAR:
			if (!unescape(output, input))
				return (true);
			break;
		case XCODEC_DECLARE_CHAR:
			try {
				if (!declare(input))
					return (true);
			} catch (uint64_t sum) {
				return (false);
			}
			break;
		case XCODEC_BACKREF_CHAR:
			if (!backreference(output, input))
				return (true);
			break;
		default:
			output->append(input->peek());
			input->skip(1);
			break;
		}
	}
	return (true);
}
