#include <common/buffer.h>
#include <common/endian.h>

#if defined(XCODEC_PIPES)
#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#endif

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_encoder.h>
#if defined(XCODEC_PIPES)
#include <xcodec/xcodec_encoder_pipe.h>
#endif
#include <xcodec/xcodec_hash.h>

typedef	std::pair<unsigned, uint64_t> offset_hash_pair_t;

XCodecEncoder::XCodecEncoder(XCodec *codec)
: log_("/xcodec/encoder"),
  cache_(codec->cache_),
  window_(),
#if defined(XCODEC_PIPES)
  pipe_(NULL),
#endif
  queued_(),
  sent_eos_(false),
  received_eos_(false)
{
	queued_.append(codec->hello());
}

XCodecEncoder::~XCodecEncoder()
{
#if defined(XCODEC_PIPES)
	ASSERT(pipe_ == NULL);
#endif
}

/*
 * This takes a view of a data stream and turns it into a series of references
 * to other data, declarations of data to be referenced, and data that needs
 * escaped.
 */
void
XCodecEncoder::encode(Buffer *output, Buffer *input)
{
	/* Process any queued HELLOs, ASKs or LEARNs.  */
	if (!queued_.empty()) {
		DEBUG(log_) << "Pushing queued data.";

		output->append(&queued_);
		queued_.clear();
	} else {
		if (!sent_eos_ && input->empty()) {
			DEBUG(log_) << "Sending <EOS>.";
			output->append(XCODEC_MAGIC);
			output->append(XCODEC_OP_EOS);
			sent_eos_ = true;
		}
	}

	if (input->empty())
		return;

	ASSERT(!sent_eos_);

	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		encode_escape(output, input, input->length());
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
		const uint8_t *p, *q = seg->end();
		for (p = seg->data(); p < q; p++) {
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
		encode_escape(output, &outq, outq.length());
	}

	ASSERT(!have_candidate);
	ASSERT(outq.empty());
	ASSERT(input->empty());
}

/*
 * Encode an ASK of the remote XCodec.
 */
void
XCodecEncoder::encode_ask(uint64_t hash)
{
	uint64_t behash = BigEndian::encode(hash);

	queued_.append(XCODEC_MAGIC);
	queued_.append(XCODEC_OP_ASK);
	queued_.append((const uint8_t *)&behash, sizeof behash);
}

/*
 * Encode a LEARN for the remote XCodec.
 */
void
XCodecEncoder::encode_learn(BufferSegment *seg)
{
	queued_.append(XCODEC_MAGIC);
	queued_.append(XCODEC_OP_LEARN);
	queued_.append(seg);
}

/*
 * Signal that output is ready if there is some that has been queued.
 */
void
XCodecEncoder::encode_push(void)
{
#if defined(XCODEC_PIPES)
	if (!queued_.empty() && pipe_ != NULL)
		pipe_->output_ready();
#endif
}

void
XCodecEncoder::received_eos(void)
{
	if (received_eos_) {
		DEBUG(log_) << "Received duplicate <EOS> from decoder.";
	}
	received_eos_ = true;
}

#if defined(XCODEC_PIPES)
/*
 * Set or clear the association with an XCodecPipe.
 */
void
XCodecEncoder::set_pipe(XCodecEncoderPipe *pipe)
{
	ASSERT(pipe != NULL || pipe_ != NULL);
	ASSERT(pipe_ == NULL || pipe == NULL);

	pipe_ = pipe;

	if (pipe_ != NULL && !queued_.empty())
		pipe_->output_ready();
}
#endif

void
XCodecEncoder::encode_declaration(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, BufferSegment **segp)
{
	if (offset != 0) {
		encode_escape(output, input, offset);
	}

	BufferSegment *nseg;
	input->copyout(&nseg, XCODEC_SEGMENT_LENGTH);

	cache_->enter(hash, nseg);

	output->append(XCODEC_MAGIC);
	output->append(XCODEC_OP_EXTRACT);
	output->append(nseg);

	window_.declare(hash, nseg);
	if (segp == NULL)
		nseg->unref();

	/*
	 * Skip to the end.
	 */
	input->skip(XCODEC_SEGMENT_LENGTH);

	if (segp != NULL)
		*segp = nseg;
}

void
XCodecEncoder::encode_escape(Buffer *output, Buffer *input, unsigned length)
{
	do {
		unsigned offset;
		if (!input->find(XCODEC_MAGIC, &offset, length)) {
			output->append(input, length);
			input->skip(length);
			return;
		}

		if (offset != 0) {
			output->append(input, offset);
			length -= offset;
			input->skip(offset);
		}

		output->append(XCODEC_MAGIC);
		output->append(XCODEC_OP_ESCAPE);

		length -= sizeof XCODEC_MAGIC;
		input->skip(sizeof XCODEC_MAGIC);
	} while (length != 0);
}

bool
XCodecEncoder::encode_reference(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, BufferSegment *oseg)
{
	uint8_t data[XCODEC_SEGMENT_LENGTH];
	input->copyout(data, offset, sizeof data);

	if (!oseg->equal(data, sizeof data))
		return (false);

	if (offset != 0) {
		encode_escape(output, input, offset);
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
		output->append(XCODEC_MAGIC);
		output->append(XCODEC_OP_BACKREF);
		output->append(b);
	} else {
		output->append(XCODEC_MAGIC);
		output->append(XCODEC_OP_REF);
		uint64_t behash = BigEndian::encode(hash);
		output->append((const uint8_t *)&behash, sizeof behash);

		window_.declare(hash, oseg);
	}

	return (true);
}
