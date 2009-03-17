#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcbackref.h>
#include <xcodec/xcdb.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_slice.h>

struct xcodec_special_p {
	bool operator() (uint8_t ch) const
	{
		return (XCODEC_CHAR_SPECIAL(ch));
	}
};

XCodecSlice::XCodecSlice(XCDatabase *database, Buffer *input, XCodecSlice *next)
: log_("/xcodec/slice"),
  database_(database),
  type_(XCodecSlice::EscapeData),
  prefix_(),
  hash_(0),
  seg_(NULL),
  suffix_(),
  next_(next)
{
	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		prefix_.append(input);
		input->clear();
		return;
	}

	if (input->length() >= XCODEC_SLICE_SIZE) {
		Buffer slice;
		slice.append(input, XCODEC_SLICE_SIZE);
		input->skip(XCODEC_SLICE_SIZE);

		process(&slice);
		ASSERT(slice.empty());
	}

	if (input->empty())
		return;

	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		suffix_.append(input);
		input->clear();
		return;
	}

	next_ = new XCodecSlice(database, input, next_);
}

XCodecSlice::~XCodecSlice()
{
	if (seg_ != NULL) {
		seg_->unref();
		seg_ = NULL;
	}

	if (next_ != NULL) {
		delete next_;
		next_ = NULL;
	}
}

void
XCodecSlice::process(Buffer *input)
{
	ASSERT(type_ == XCodecSlice::EscapeData);
	ASSERT(input->length() >= XCODEC_SEGMENT_LENGTH);

	/*
	 * XXX
	 * Change type_ based on what we can find, etc., etc.
	 */
	suffix_.append(input);
	input->clear();
}

void
XCodecSlice::encode(XCBackref *backref, Buffer *output) const
{
	uint64_t lehash;
	uint8_t b;

	if (!prefix_.empty()) {
		Buffer prefix(prefix_);
		prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(prefix);
	}

	switch (type_) {
	case XCodecSlice::HashDeclare:
		output->append(XCODEC_DECLARE_CHAR);
		lehash = LittleEndian::encode(hash_);
		output->append((const uint8_t *)&lehash, sizeof lehash);
		output->append(seg_);

		backref->declare(hash_, seg_);
		/*
		 * Now process a reference.  XXX Now that declarations are
		 * at the appropriate location, maybe we should have them be
		 * implicit references.  On the other hand, maybe we should
		 * send out the declarations earlier.
		 */
	case XCodecSlice::HashReference:
		if (backref->present(hash_, &b)) {
			output->append(XCODEC_BACKREF_CHAR);
			output->append(b);
		} else {
			output->append(XCODEC_HASHREF_CHAR);
			lehash = LittleEndian::encode(hash_);
			output->append((const uint8_t *)&lehash, sizeof lehash);
		}
		break;
	default:
		break;
	}

	if (!suffix_.empty()) {
		Buffer suffix(suffix_);
		suffix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(suffix);
	}

	if (next_ != NULL)
		next_->encode(backref, output);
}
