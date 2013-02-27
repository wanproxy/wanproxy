/*
 * Copyright (c) 2009-2011 Juli Mallett. All rights reserved.
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
 */
void
XCodecEncoder::encode(Buffer *output, Buffer *input)
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
			DEBUG(log_) << "Buffer couldn't yield a hash.";
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
				BufferSegment *nseg;
				encode_declaration(output, &outq, candidate.offset_, candidate.symbol_, &nseg);

				o -= candidate.offset_ + XCODEC_SEGMENT_LENGTH;
				start = o - XCODEC_SEGMENT_LENGTH;

				candidate.set_ = false;

				/*
				 * If, on top of that, the just-declared hash is
				 * the same as the current hash, consider referencing
				 * it immediately.
				 */
				if (hash == candidate.symbol_) {
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
					xcodec_hash.reset();

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
		encode_declaration(output, &outq, candidate.offset_, candidate.symbol_, NULL);
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
XCodecEncoder::encode_declaration(Buffer *output, Buffer *input, unsigned offset, uint64_t hash, BufferSegment **segp)
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
		if (!encode_reference(output, input, 0, hash, nseg)) /* XXX Pass NULL not nseg to skip check?  */
			NOTREACHED(log_);
		if (segp == NULL)
			nseg->unref();
		else
			*segp = nseg;
		return;
	}

	/*
	 * Declarations are extracted in-band.
	 */
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
	ASSERT(log_, length != 0);

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
		output->append(&behash);

		window_.declare(hash, oseg);
	}

	return (true);
}
