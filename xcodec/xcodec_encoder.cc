#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_hash.h>

typedef	std::pair<unsigned, uint64_t> offset_hash_pair_t;
typedef	std::pair<unsigned, BufferSegment *> offset_seg_pair_t;

struct xcodec_special_p {
	bool operator() (uint8_t ch) const
	{
		return (XCODEC_CHAR_SPECIAL(ch));
	}
};

XCodecEncoder::XCodecEncoder(XCodec *codec)
: log_("/xcodec/encoder"),
  cache_(codec->cache_),
  window_()
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
	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		input->escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(input);
		input->clear();
		return;
	}

	XCodecHash<XCODEC_SEGMENT_LENGTH> xcodec_hash;
	std::deque<offset_hash_pair_t> offset_hash_map;
	Buffer outq;
	unsigned o = 0;

	while (!input->empty()) {
		BufferSegment *seg;
		input->moveout(&seg);

		outq.append(seg);

		const uint8_t *p;
		for (p = seg->data(); p < seg->end(); p++) {
			xcodec_hash.roll(*p);
			if (++o < XCODEC_SEGMENT_LENGTH)
				continue;

			unsigned start = o - XCODEC_SEGMENT_LENGTH;
			uint64_t hash = xcodec_hash.mix();
			bool hash_collision;

			BufferSegment *oseg = cache_->lookup(hash);
			if (oseg != NULL) {
				/*
				 * This segment already exists.  If it's
				 * identical to this chunk of data, then that's
				 * positively fantastic.
				 */
				uint8_t data[XCODEC_SEGMENT_LENGTH];
				outq.copyout(data, start, sizeof data);

				if (oseg->match(data, sizeof data)) {
					if (start != 0) {
						Buffer prefix;
						outq.moveout(&prefix, 0, start);

						prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
						output->append(prefix);
						prefix.clear();
					}

					/*
					 * Skip to the end.
					 */
					outq.skip(XCODEC_SEGMENT_LENGTH); 

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

					oseg->unref();

					o = 0;
					offset_hash_map.clear();

					continue;
				}
				oseg->unref();
				DEBUG(log_) << "Collision in first pass.";
				hash_collision = true;

				/*
				 * Fall through to defining the previous hash if
				 * it is appropriate to do so.
				 */
			} else {
				hash_collision = false;
			}

			if (!offset_hash_map.empty()) {
				/*
				 * If there is a previous hash in the
				 * offset-hash map that would overlap with this
				 * hash, then we have no reason to remember this
				 * hash for later.
				 */
				const offset_hash_pair_t& last = offset_hash_map.back();

				if (last.first + XCODEC_SEGMENT_LENGTH > start) {
					/* We might still find an alternative.  */
					continue;
				}

				uint8_t data[XCODEC_SEGMENT_LENGTH];
				outq.copyout(data, last.first, sizeof data);

				/*
				 * No hit is fantastic, too -- go ahead and
				 * declare this hash.
				 */
				BufferSegment *nseg = new BufferSegment();
				nseg->append(data, sizeof data);

				cache_->enter(last.second, nseg);

				if (last.first != 0) {
					Buffer prefix;
					outq.moveout(&prefix, 0, last.first);

					prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
					output->append(prefix);
					prefix.clear();
				}

				output->append(XCODEC_DECLARE_CHAR);
				uint64_t lehash = LittleEndian::encode(last.second);
				output->append((const uint8_t *)&lehash, sizeof lehash);
				output->append(nseg);

				window_.declare(last.second, nseg);

				/*
				 * Skip to the end.
				 */
				outq.skip(XCODEC_SEGMENT_LENGTH); 

				/*
				 * And output a reference.
				 */
				uint8_t b;
				if (window_.present(last.second, &b)) {
					output->append(XCODEC_BACKREF_CHAR);
					output->append(b);
				} else {
					NOTREACHED();
				}

				o -= last.first + XCODEC_SEGMENT_LENGTH;
				start = o - XCODEC_SEGMENT_LENGTH;

				offset_hash_map.clear();

				/*
				 * If this hash is the same as the has for
				 * the current data, then make sure that they
				 * are compatible before remembering the
				 * current hash
				 */
				if (!hash_collision && hash == last.second) {
					outq.copyout(data, start, sizeof data);

					if (!nseg->match(data, sizeof data)) {
						nseg->unref();
						DEBUG(log_) << "Collision in adjacent-declare pass.";
						continue;
					}
					nseg->unref();

					DEBUG(log_) << "Hit in adjacent-declare pass.";

					if (start != 0) {
						Buffer prefix;
						outq.moveout(&prefix, 0, start);

						prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
						output->append(prefix);
						prefix.clear();
					}

					/*
					 * Skip to the end.
					 */
					outq.skip(XCODEC_SEGMENT_LENGTH);

					output->append(XCODEC_BACKREF_CHAR);
					output->append(b);

					o = 0;

					continue;
				}

				nseg->unref();

				/*
				 * Remember the current hash.
				 */
			}

			/*
			 * No collision, remember this for later.
			 */
			if (!hash_collision) {
				offset_hash_pair_t ohp;
				ohp.first = start;
				ohp.second = hash;
				offset_hash_map.push_back(ohp);
			}

		}

		seg->unref();
	}

	/*
	 * There's a hash we can declare, do it.
	 */
	if (!offset_hash_map.empty()) {
		const offset_hash_pair_t& last = offset_hash_map.back();

		uint8_t data[XCODEC_SEGMENT_LENGTH];
		outq.copyout(data, last.first, sizeof data);

		/*
		 * No hit is fantastic, too -- go ahead and
		 * declare this hash.
		 */
		BufferSegment *nseg = new BufferSegment();
		nseg->append(data, sizeof data);

		cache_->enter(last.second, nseg);

		if (last.first != 0) {
			Buffer prefix;
			outq.moveout(&prefix, 0, last.first);

			prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
			output->append(prefix);
			prefix.clear();
		}

		output->append(XCODEC_DECLARE_CHAR);
		uint64_t lehash = LittleEndian::encode(last.second);
		output->append((const uint8_t *)&lehash, sizeof lehash);
		output->append(nseg);

		window_.declare(last.second, nseg);

		/*
		 * Skip to the end.
		 */
		outq.skip(XCODEC_SEGMENT_LENGTH); 

		/*
		 * And output a reference.
		 */
		uint8_t b;
		if (window_.present(last.second, &b)) {
			output->append(XCODEC_BACKREF_CHAR);
			output->append(b);
		} else {
			NOTREACHED();
		}
	}

	if (!outq.empty()) {
		Buffer suffix(outq);
		outq.clear();

		suffix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());

		output->append(suffix);
		outq.clear();
	}

	ASSERT(outq.empty());
	ASSERT(input->empty());
}
