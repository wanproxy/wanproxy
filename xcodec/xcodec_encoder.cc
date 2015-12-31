/*
 * Copyright (c) 2009-2014 Juli Mallett. All rights reserved.
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

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>

struct candidate_symbol {
	bool set_;
	unsigned offset_;
	uint64_t symbol_;
};

XCodecEncoder::XCodecEncoder(XCodecCache *cache)
: log_("/xcodec/encoder"),
  cache_(cache),
  window_(),
  stream_(!cache_->out_of_band())
{ }

XCodecEncoder::~XCodecEncoder()
{ }

/*
 * This takes a view of a data stream and turns it into a series of references
 * to other data, declarations of data to be referenced, and data that needs
 * escaped.
 *
 * The caller may request to retain a map of hashes and segments referenced by
 * this stream, a "refmap".
 *
 * Note that although each refmap is local to the frame referencing it, there
 * is still an issue with how hash reuse within even a single call to this
 * function could occur.  Consider the case of a memory cache limited to 2-3
 * segments.  When a colliding chunk of data is discovered, we will collide
 * within the refmap due to eviction.  The stream's decoding should be
 * decidable, but it is so only if we track an ordered listing of each
 * reference in order and the data it corresponds to and have some way of
 * identifying <ASK>/<LEARN> type requests linearly.  Another solution is
 * needed here, namely to just fail and fail tremendously when we have such
 * a malformed stream of data that we can't guarantee its decoding.  Or we
 * may just want to check the refmap in addition to the backing cache (and
 * maybe in preference to it) to prevent this problem.  But that shifts part
 * of the burden for correct decoding to the decoder, which then has to be
 * more careful about assuming that a <LEARN> covers all of the data it is
 * trying to decode which requests a hash, rather than just one frame.
 */
void
XCodecEncoder::encode(Buffer *output, Buffer *input, std::map<uint64_t, BufferSegment *> *refmap)
{
	if (input->empty())
		return;

	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		encode_escape(output, input, input->length());
		return;
	}

	XCodecHash xcodec_hash;
	candidate_symbol candidate;
	Buffer outq;
	unsigned o = 0;

	candidate.set_ = false;

	/*
	 * While there is input.
	 */
	while (!input->empty()) {
		/*
		 * If we cannot acquire a complete hash within this segment,
		 * stop looking.
		 */
		if (o + input->length() < XCODEC_SEGMENT_LENGTH) {
			input->moveout(&outq);
			break;
		}

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
			ptrdiff_t resid = q - p;

			/*
			 * If we cannot acquire a complete hash within this segment.
			 */
			if (o + resid < XCODEC_SEGMENT_LENGTH) {
				/*
				 * Hash all of the bytes from it and continue.
				 */
				o += resid;
				while (p < q)
					xcodec_hash.add(*p++);
				break;
			}

			/*
			 * If we don't have a complete hash.
			 */
			if (o < XCODEC_SEGMENT_LENGTH) {
				for (;;) {
					/*
					 * Add bytes to the hash.
					 */
					xcodec_hash.add(*p);

					/*
					 * Until we have a complete hash.
					 */
					if (++o == XCODEC_SEGMENT_LENGTH)
						break;

					/*
					 * Go to the next byte.
					 */
					p++;
				}
				ASSERT(log_, o == XCODEC_SEGMENT_LENGTH);
			} else {
				/*
				 * Roll it into the rolling hash.
				 */
				xcodec_hash.roll(*p);
				o++;
			}

			ASSERT(log_, o >= XCODEC_SEGMENT_LENGTH);
			ASSERT(log_, p != q);

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
			if (candidate.set_ && candidate.offset_ + XCODEC_SEGMENT_LENGTH <= start) {
				encode_declaration(output, &outq, candidate.offset_, candidate.symbol_);

				o -= candidate.offset_ + XCODEC_SEGMENT_LENGTH;
				start = o - XCODEC_SEGMENT_LENGTH;

				candidate.set_ = false;
			}

			/*
			 * Now attempt to encode this hash as a reference if it
			 * has been defined before.
			 */
			if (find_reference(output, &outq, start, hash, refmap)) {
				o = 0;
				xcodec_hash.reset();

				/*
				 * We have output any data before this hash
				 * in escaped form, so any candidate hash
				 * before it is invalid now.
				 */
				candidate.set_ = false;
				continue;
			}

			/*
			 * Not defined before, it's a candidate for declaration
			 * if we don't already have one.
			 */
			if (candidate.set_) {
				/*
				 * We already have a hash that occurs earlier,
				 * isn't a collision and includes data that's
				 * covered by this hash, so don't remember it
				 * and keep going.
				 */
				ASSERT(log_, candidate.offset_ + XCODEC_SEGMENT_LENGTH > start);
				continue;
			}

			/*
			 * The hash at this offset doesn't collide with any
			 * other and is the first viable hash we've seen so far
			 * in the stream, so remember it so that if we don't
			 * find something to reference we can declare this one
			 * for future use.
			 */
			candidate.offset_ = start;
			candidate.symbol_ = hash;
			candidate.set_ = true;
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
	if (candidate.set_) {
		ASSERT(log_, !outq.empty());
		encode_declaration(output, &outq, candidate.offset_, candidate.symbol_);
		candidate.set_ = false;
	}

	/*
	 * There's data after that hash or no candidate hash, so
	 * just escape it.
	 */
	if (!outq.empty()) {
		encode_escape(output, &outq, outq.length());
	}

	ASSERT(log_, !candidate.set_);
	ASSERT(log_, outq.empty());
	ASSERT(log_, input->empty());
}

void
XCodecEncoder::encode_declaration(Buffer *output, Buffer *input, unsigned offset, uint64_t hash)
{
	if (offset != 0) {
		encode_escape(output, input, offset);
	}

	BufferSegment *nseg;
	input->copyout(&nseg, XCODEC_SEGMENT_LENGTH);

	cache_->enter(hash, nseg);

	if (!stream_) {
		/*
		 * Declarations occur out-of-band.
		 */
		encode_reference(output, input, 0, hash, nseg, NULL);
		nseg->unref();
		return;
	}

	/*
	 * Declarations are extracted in-band.
	 */
	output->append(XCODEC_MAGIC);
	output->append(XCODEC_OP_EXTRACT);
	output->append(nseg);

	bool collision = window_.declare(hash, nseg);
	if (collision)
		DEBUG(log_) << "Collision in encoder window; cleared old entry.";
	nseg->unref();

	/*
	 * Skip to the end.
	 */
	input->skip(XCODEC_SEGMENT_LENGTH);
}

void
XCodecEncoder::encode_escape(Buffer *output, Buffer *input, unsigned length)
{
	ASSERT_NON_ZERO(log_, length);

	do {
		size_t offset;
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

void
XCodecEncoder::encode_reference(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, BufferSegment *oseg, std::map<uint64_t, BufferSegment *> *refmap)
{
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
	output->append(XCODEC_MAGIC);
	output->append(XCODEC_OP_REF);
	uint64_t behash = BigEndian::encode(hash);
	output->append(&behash);

	window_.declare(hash, oseg);

	if (refmap != NULL) {
		std::map<uint64_t, BufferSegment *>::const_iterator it;
		it = refmap->find(hash);
		if (it == refmap->end()) {
			oseg->ref();
			refmap->insert(std::map<uint64_t, BufferSegment *>::value_type(hash, oseg));
		}
	}
}

bool
XCodecEncoder::find_reference(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, std::map<uint64_t, BufferSegment *> *refmap)
{
	/*
	 * Now check in the cache proper.
	 */
	BufferSegment *oseg = cache_->lookup(hash);
	if (oseg != NULL) {
		uint8_t data[XCODEC_SEGMENT_LENGTH];
		input->copyout(data, offset, sizeof data);

		/*
		 * This segment already exists.  If it's
		 * identical to this chunk of data, then that's
		 * positively fantastic.
		 */
		if (!oseg->equal(data, sizeof data)) {
			/*
			 * This hash isn't usable because it collides
			 * with another, so keep looking for something
			 * viable.
			 *
			 * XXX
			 * If this is the first hash (i.e.
			 * !candidate.set_) then we can adjust the
			 * start of the current window and escape the
			 * first byte right away.  Does that help?
			 */
			oseg->unref();
			DEBUG(log_) << "Collision in first pass.";
			return (false);
		}

		encode_reference(output, input, offset, hash, oseg, refmap);
		oseg->unref();
		return (true);
	}

	return (false);
}
