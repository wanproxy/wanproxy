#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcbackref.h>
#include <xcodec/xcdb.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder.h>

typedef	std::pair<unsigned, uint64_t> offset_hash_pair_t;
typedef	std::pair<unsigned, BufferSegment *> offset_seg_pair_t;

struct xcodec_special_p {
	bool operator() (uint8_t ch) const
	{
		return (XCODEC_CHAR_SPECIAL(ch));
	}
};

XCodecEncoder::Data::Data(void)
: prefix_(),
  hash_(),
  seg_(NULL)
{ }

XCodecEncoder::Data::Data(const XCodecEncoder::Data& src)
: prefix_(src.prefix_),
  hash_(src.hash_),
  seg_(NULL)
{
	if (src.seg_ != NULL) {
		src.seg_->ref();
		seg_ = src.seg_;
	}
}

XCodecEncoder::Data::~Data()
{
	if (seg_ != NULL) {
		seg_->unref();
		seg_ = NULL;
	}
}


XCodecEncoder::XCodecEncoder(XCodec *codec)
: log_("/xcodec/encoder"),
  database_(codec->database_),
  backref_()
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

	XCHash<XCODEC_SEGMENT_LENGTH> xcodec_hash;
	std::deque<offset_hash_pair_t> offset_hash_map;
	std::deque<offset_seg_pair_t> offset_seg_map;
	Buffer outq;
	unsigned o = 0;
	unsigned base = 0;

	while (!input->empty()) {
		BufferSegment *seg;
		input->moveout(&seg);

		outq.append(seg);

		const uint8_t *p;
		for (p = seg->data(); p < seg->end(); p++) {
			if (++o < base)
				continue;

			xcodec_hash.roll(*p);

			if (o - base < XCODEC_SEGMENT_LENGTH)
				continue;

			unsigned start = o - XCODEC_SEGMENT_LENGTH;
			uint64_t hash = xcodec_hash.mix();

			/*
			 * XXX
			 * We can cache presence/non-presence within this part
			 * of the stream to reduce the size of the map we have
			 * to look up in.  Doing so should help a lot.
			 */
			BufferSegment *oseg;
			oseg = database_->lookup(hash);
			if (oseg != NULL) {
				/*
				 * This segment already exists.  If it's
				 * identical to this chunk of data, then that's
				 * positively fantastic.
				 */
				uint8_t data[XCODEC_SEGMENT_LENGTH];
				outq.copyout(data, start, sizeof data);

				if (!oseg->match(data, sizeof data)) {
					oseg->unref();
					DEBUG(log_) << "Collision in first pass.";
					continue;
				}

				/*
				 * The segment was identical, we can use it.
				 * We're giving our reference to the offset-seg
				 * map.
				 */
				offset_seg_pair_t osp;
				osp.first = start;
				osp.second = oseg;
				offset_seg_map.push_back(osp);

				/* Do not hash any data until after us.  */
				base = o;
			} else {
				/*
				 * If there is a previous hash in the
				 * offset-hash map that would overlap with this
				 * hash, then we have no reason to remember this
				 * hash for later -- unless there are some
				 * in-stream collisions.  Which is bad, but hard
				 * to avoid and fairly uncommon.
				 */
				if (!offset_hash_map.empty()) {
					const offset_hash_pair_t& last = offset_hash_map.back();

					if (last.first + XCODEC_SEGMENT_LENGTH > start) {
						/*
						 * The last hash in the
						 * offset-hash map has us
						 * covered; don't record this
						 * hash.
						 */
						continue;
					} else {
						/*
						 * XXX
						 * We could go ahead and declare
						 * the last hash now, but that
						 * is actually kind of bad for
						 * performance, even though it
						 * avoids the in-stream
						 * collision problem.
						 *
						 * It would probably be less bad
						 * for performance if we got rid
						 * of the next loop, which is
						 * probably possible if we play
						 * our cards right.
						 */
					}
				}
			}

			/*
			 * No collision, remember this for later.
			 */
			offset_hash_pair_t ohp;
			ohp.first = start;
			ohp.second = hash;
			offset_hash_map.push_back(ohp);
		}
		seg->unref();
	}

	/*
	 * Now compile the offset-hash map into child data.
	 */

	std::deque<offset_hash_pair_t>::iterator ohit;
	BufferSegment *seg;
	unsigned soff = 0;
	while ((ohit = offset_hash_map.begin()) != offset_hash_map.end()) {
		unsigned start = ohit->first;
		uint64_t hash = ohit->second;
		unsigned end = start + XCODEC_SEGMENT_LENGTH;

		/*
		 * We only get one bite at the apple.
		 */
		offset_hash_map.erase(ohit);

		std::deque<offset_seg_pair_t>::iterator osit = offset_seg_map.begin();
		Data slice;

		/*
		 * If this offset-hash corresponds to this offset-segment, use
		 * it!
		 */
		if (osit != offset_seg_map.end()) {
			if (start == osit->first) {
				slice.hash_ = hash;
				/* The slice holds our reference.  */
				slice.seg_ = osit->second;

				/*
				 * Dispose of this entry.
				 */
				offset_seg_map.erase(osit);
			} else if (start < osit->first && end > osit->first) {
				/*
				 * This hash would overlap with a
				 * offset-segment.  Skip it.
				 */
				continue;
			} else {
				/*
				 * There is an offset-segment in our distant
				 * future, we can try this hash for now.
				 */
			}
		} else {
			/*
			 * There is no offset-segment after this, so we can just
			 * use this hash gleefully.
			 */
		}

		/*
		 * We have not yet set a seg_ in this Data, so it's time for us
		 * to declare this segment.
		 */
		if (slice.seg_ == NULL) {
			uint8_t data[XCODEC_SEGMENT_LENGTH];
			outq.copyout(data, start - soff, sizeof data);

			/*
			 * We can't assume that this isn't in the database.
			 * Since we're declaring things all the time in this
			 * stream, we may have introduced hits and collisions.
			 * So we, sadly, have to go back to the well.
			 */
			seg = database_->lookup(hash);
			if (seg != NULL) {
				if (!seg->match(data, sizeof data)) {
					seg->unref();
					DEBUG(log_) << "Collision in second pass.";
					continue;
				}

				/*
				 * A hit!  Well, that's fantastic.
				 */
				slice.hash_ = hash;
				/* The slice holds our reference.  */
				slice.seg_ = seg;
			} else {
				/*
				 * No hit is fantastic, too -- go ahead and
				 * declare this hash.
				 */
				seg = new BufferSegment();
				seg->append(data, sizeof data);

				database_->enter(hash, seg);

				slice.hash_ = hash;
				/* The slice holds our reference.  */
				slice.seg_ = seg;

				output->append(XCODEC_DECLARE_CHAR);
				uint64_t lehash = LittleEndian::encode(slice.hash_);
				output->append((const uint8_t *)&lehash, sizeof lehash);
				output->append(slice.seg_);

				backref_.declare(slice.hash_, slice.seg_);
			}

			/*
			 * Skip any successive overlapping hashes.
			 */
			while ((ohit = offset_hash_map.begin()) != offset_hash_map.end()) {
				if (ohit->first >= end)
					break;
				offset_hash_map.erase(ohit);
			}
		} else {
			/*
			 * There should not be any successive overlapping hashes
			 * if we found a hit in the first pass.  XXX We should
			 * ASSERT that this looks like we'd expect.
			 */
		}

		ASSERT(slice.seg_ != NULL);

		/*
		 * Copy out any prefixing data.
		 */
		if (soff != start) {
			outq.moveout(&slice.prefix_, 0, start - soff);
			soff = start;

			slice.prefix_.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
			output->append(slice.prefix_);
			slice.prefix_.clear();
		}

		/*
		 * And skip this segment.
		 */
		outq.skip(XCODEC_SEGMENT_LENGTH); 
		soff = end;

		/*
		 * And output a reference.
		 */
		uint8_t b;
		if (backref_.present(slice.hash_, &b)) {
			output->append(XCODEC_BACKREF_CHAR);
			output->append(b);
		} else {
			output->append(XCODEC_HASHREF_CHAR);
			uint64_t lehash = LittleEndian::encode(slice.hash_);
			output->append((const uint8_t *)&lehash, sizeof lehash);

			backref_.declare(slice.hash_, slice.seg_);
		}
	}

	/*
	 * The segment map should be empty, too.  It should only have entries
	 * that correspond to offset-hash entries.
	 */
	ASSERT(offset_seg_map.empty());

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
