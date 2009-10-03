#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>

typedef	std::pair<unsigned, uint64_t> offset_hash_pair_t;

struct xcodec_special_p {
	bool operator() (uint8_t ch) const
	{
		return (XCODEC_CHAR_SPECIAL(ch));
	}
};

XCodecEncoder::XCodecEncoder(XCodec *codec)
: log_("/xcodec/encoder"),
  cache_(codec->cache_),
  window_(),
  input_bytes_(0),
  output_bytes_(0)
{ }

XCodecEncoder::~XCodecEncoder()
{
	DEBUG(log_) << "Input bytes: " << input_bytes_ << "; Output bytes: " << output_bytes_;
}

/*
 * This takes a view of a data stream and turns it into a series of references
 * to other data, declarations of data to be referenced, and data that needs
 * escaped.
 */
void
XCodecEncoder::encode(Buffer *output, Buffer *input)
{
	uint64_t output_initial_size = output->length();

	input_bytes_ += input->length();

	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		input->escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(input);
		input->clear();

		output_bytes_ += output->length() - output_initial_size;
		return;
	}

	XCodecHash<XCODEC_SEGMENT_LENGTH> xcodec_hash;
	offset_hash_pair_t candidate;
	bool have_candidate;
	Buffer outq;
	unsigned o = 0;

	have_candidate = false;

	/*
	 * While there is input.
	 */
	while (!input->empty()) {
		/*
		 * Take the first BufferSegment out of the input Buffer.
		 */
		BufferSegment *seg;
		input->moveout(&seg);

		/*
		 * And add it to a temporary Buffer where input is queued.
		 */
		outq.append(seg);

		/*
		 * And for every byte in this BufferSegment.
		 */
		const uint8_t *p;
		for (p = seg->data(); p < seg->end(); p++) {
			/*
			 * Add it to the rolling hash.
			 */
			xcodec_hash.roll(*p);

			/*
			 * If the rolling hash does not cover an entire
			 * segment yet, just keep adding to it.
			 */
			if (++o < XCODEC_SEGMENT_LENGTH)
				continue;

			/*
			 * And then mix the hash's internal state into a
			 * uint64_t that we can use to refer to that data
			 * and to look up possible past occurances of that
			 * data in the XCodecCache.
			 */
			unsigned start = o - XCODEC_SEGMENT_LENGTH;
			uint64_t hash = xcodec_hash.mix();

			/*
			 * If there is a pending candidate hash that wouldn't
			 * overlap with the data that the rolling hash presently
			 * covers, declare it now.
			 */
			if (have_candidate && candidate.first + XCODEC_SEGMENT_LENGTH <= start) {
				BufferSegment *nseg;
				encode_declaration(output, &outq, candidate.first, candidate.second, &nseg);

				o -= candidate.first + XCODEC_SEGMENT_LENGTH;
				start = o - XCODEC_SEGMENT_LENGTH;

				have_candidate = false;

				/*
				 * If, on top of that, the just-declared hash is
				 * the same as the current hash, consider referencing
				 * it immediately.
				 */
				if (hash == candidate.second) {
					/*
					 * If it's a hash collision, though, nevermind.
					 * Skip trying to use this hash as a reference,
					 * too, and go on to the next one.
					 */
					if (!encode_reference(output, &outq, start, hash, nseg)) {
						nseg->unref();
						DEBUG(log_) << "Collision in adjacent-declare pass.";
						continue;
					}
					nseg->unref();

					/*
					 * But if there's no collection, then we can move
					 * on to looking for the *next* hash/data to declare
					 * or reference.
					 */
					o = 0;

					DEBUG(log_) << "Hit in adjacent-declare pass.";
					continue;
				}
				nseg->unref();
			}

			/*
			 * Now attempt to encode this hash as a reference if it
			 * has been defined before.
			 */
			BufferSegment *oseg = cache_->lookup(hash);
			if (oseg != NULL) {
				/*
				 * This segment already exists.  If it's
				 * identical to this chunk of data, then that's
				 * positively fantastic.
				 */
				if (encode_reference(output, &outq, start, hash, oseg)) {
					oseg->unref();

					o = 0;

					/*
					 * We have output any data before this hash
					 * in escaped form, so any candidate hash
					 * before it is invalid now.
					 */
					have_candidate = false;
					continue;
				}

				/*
				 * This hash isn't usable because it collides
				 * with another, so keep looking for something
				 * viable.
				 */
				oseg->unref();
				DEBUG(log_) << "Collision in first pass.";
				continue;
			}

			/*
			 * Not defined before, it's a candidate for declaration if
			 * we don't already have one.
			 */
			if (have_candidate) {
				/*
				 * We already have a hash that occurs earlier,
				 * isn't a collision and includes data that's
				 * covered by this hash, so don't remember it
				 * and keep going.
				 */
				ASSERT(candidate.first + XCODEC_SEGMENT_LENGTH > start);
				continue;
			}

			/*
			 * The hash at this offset doesn't collide with any
			 * other and is the first viable hash we've seen so far
			 * in the stream, so remember it so that if we don't
			 * find something to reference we can declare this one
			 * for future use.
			 */
			candidate.first = start;
			candidate.second = hash;
			have_candidate = true;
		}

		seg->unref();
	}

	/*
	 * Done processing input.  If there's still data in the outq Buffer,
	 * then we need to declare any candidate hashes and escape any data
	 * after them.
	 */

	/*
	 * There's a hash we can declare, do it.
	 */
	if (have_candidate) {
		ASSERT(!outq.empty());
		encode_declaration(output, &outq, candidate.first, candidate.second, NULL);
		have_candidate = false;
	}

	/*
	 * There's data after that hash or no candidate hash, so
	 * just escape it.
	 */
	if (!outq.empty()) {
		Buffer suffix(outq);
		outq.clear();

		suffix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());

		output->append(suffix);
		outq.clear();
	}

	ASSERT(!have_candidate);
	ASSERT(outq.empty());
	ASSERT(input->empty());

	output_bytes_ += output->length() - output_initial_size;
}

void
XCodecEncoder::encode_declaration(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, BufferSegment **segp)
{
	if (offset != 0) {
		Buffer prefix;
		input->moveout(&prefix, 0, offset);

		prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(prefix);
		prefix.clear();
	}

	BufferSegment *nseg;
	input->copyout(&nseg, XCODEC_SEGMENT_LENGTH);

	cache_->enter(hash, nseg);

	output->append(XCODEC_DECLARE_CHAR);
	uint64_t lehash = LittleEndian::encode(hash);
	output->append((const uint8_t *)&lehash, sizeof lehash);
	output->append(nseg);

	window_.declare(hash, nseg);
	if (segp == NULL)
		nseg->unref();

	/*
	 * Skip to the end.
	 */
	input->skip(XCODEC_SEGMENT_LENGTH); 

	/*
	 * And output a reference.
	 */
	uint8_t b;
	if (window_.present(hash, &b)) {
		output->append(XCODEC_BACKREF_CHAR);
		output->append(b);
	} else {
		NOTREACHED();
	}

	if (segp != NULL)
		*segp = nseg;
}

bool
XCodecEncoder::encode_reference(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, BufferSegment *oseg)
{
	uint8_t data[XCODEC_SEGMENT_LENGTH];
	input->copyout(data, offset, sizeof data);

	if (!oseg->match(data, sizeof data))
		return (false);

	if (offset != 0) {
		Buffer prefix;
		input->moveout(&prefix, 0, offset);

		prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(prefix);
		prefix.clear();
	}

	/*
	 * Skip to the end.
	 */
	input->skip(XCODEC_SEGMENT_LENGTH); 

	/*
	 * And output a reference.
	 */
	uint8_t b;
	if (window_.present(hash, &b)) {
		output->append(XCODEC_BACKREF_CHAR);
		output->append(b);
	} else {
		output->append(XCODEC_HASHREF_CHAR);
		uint64_t lehash = LittleEndian::encode(hash);
		output->append((const uint8_t *)&lehash, sizeof lehash);

		window_.declare(hash, oseg);
	}

	return (true);
}
