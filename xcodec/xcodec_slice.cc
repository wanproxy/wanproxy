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

XCodecSlice::Data::Data(void)
: prefix_(),
  hash_(),
  seg_(NULL)
{ }

XCodecSlice::Data::Data(const XCodecSlice::Data& src)
: prefix_(src.prefix_),
  hash_(src.hash_),
  seg_(NULL)
{
	if (src.seg_ != NULL) {
		src.seg_->ref();
		seg_ = src.seg_;
	}
}

XCodecSlice::Data::~Data()
{
	if (seg_ != NULL) {
		seg_->unref();
		seg_ = NULL;
	}
}

XCodecSlice::XCodecSlice(XCDatabase *database, Buffer *input)
: log_("/xcodec/slice"),
  database_(database),
  prefix_(),
  data_(),
  suffix_()
{
	if (input->length() < XCODEC_SEGMENT_LENGTH) {
		prefix_.append(input);
		input->clear();
		return;
	}

	process(input);
	ASSERT(input->empty());
}

XCodecSlice::~XCodecSlice()
{ }

/*
 * This takes a view of a data stream and turns it into a series of references
 * to other data, declarations of data to be referenced, and data that needs
 * escaped.
 *
 * It's very simple and aggressive and performs worse than the original encoder
 * as a result.  The old encoder did a few important things differently.  First,
 * it would start declaring data as soon as we knew we hadn't found a hash for a
 * given chunk of data.  What I mean is that it would look at a segment and a
 * segment but one worth of data until it found either a match or something that
 * didn't collide, and then it would use that.
 *
 * Secondly, as a result of that, it didn't need to do two database lookups, and
 * those things get expensive.
 *
 * However, we have a few places where we can do better.  First, we can emulate
 * the aggressive declaration (which is better for data and for speed) by
 * looking at the first ohit and seeing when we're a segment length away from
 * it, and then scanning it then and there to find something we can use.  Then
 * we can just put it into the offset_seg_map and we can avoid using the
 * hash_map later on.
 *
 * Probably a lot of other things.
 */
void
XCodecSlice::process(Buffer *input)
{
	ASSERT(prefix_.empty());
	ASSERT(suffix_.empty());
	ASSERT(input->length() >= XCODEC_SEGMENT_LENGTH);

	XCHash<XCODEC_SEGMENT_LENGTH> xcodec_hash;
	std::deque<std::pair<unsigned, uint64_t> > offset_hash_map;
	std::deque<std::pair<unsigned, BufferSegment *> > offset_seg_map;
	unsigned o = 0;
	unsigned base = 0;

	while (!input->empty()) {
		BufferSegment *seg;
		input->moveout(&seg);

		suffix_.append(seg);

		const uint8_t *p;
		for (p = seg->data(); p < seg->end(); p++) {
			if (++o < base)
				continue;

			xcodec_hash.roll(*p);

			if (o - base < XCODEC_SEGMENT_LENGTH)
				continue;

			unsigned start = o - XCODEC_SEGMENT_LENGTH;
			uint64_t hash = xcodec_hash.mix();

			BufferSegment *oseg;
			oseg = database_->lookup(hash);
			if (oseg != NULL) {
				/*
				 * This segment already exists.  If it's
				 * identical to this chunk of data, then that's
				 * positively fantastic.
				 */
				uint8_t data[XCODEC_SEGMENT_LENGTH];
				suffix_.copyout(data, start, sizeof data);

				if (!oseg->match(data, sizeof data)) {
					DEBUG(log_) << "Collision in first pass.";
					continue;
				}

				/*
				 * The segment was identical, we can use it.
				 * We're giving our reference to the offset-seg
				 * map.
				 */
				std::pair<unsigned, BufferSegment *> osp;
				osp.first = start;
				osp.second = oseg;
				offset_seg_map.push_back(osp);

				/* Do not hash any data until after us.  */
				base = o;
			}

			/*
			 * No collision, remember this for later.
			 */
			std::pair<unsigned, uint64_t> ohp;
			ohp.first = start;
			ohp.second = hash;
			offset_hash_map.push_back(ohp);
		}
		seg->unref();
	}

	/*
	 * Now compile the offset-hash map into child data.
	 */

	/* Reserve room for all-references.  */
	data_.reserve(prefix_.length() / XCODEC_SEGMENT_LENGTH);

	std::deque<std::pair<unsigned, uint64_t> >::iterator ohit;
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

		std::deque<std::pair<unsigned, BufferSegment *> >::iterator osit = offset_seg_map.begin();
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
			suffix_.copyout(data, start - soff, sizeof data);

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

				/* Take a reference for the declarations_.  */
				/* XXX Make sure no dupes!  */
				seg->ref();
				declarations_.insert(hash);

				slice.hash_ = hash;
				/* The slice holds our reference.  */
				slice.seg_ = seg;
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
			suffix_.moveout(&slice.prefix_, 0, start - soff);
			soff = start;
		}

		/*
		 * And skip this segment.
		 */
		suffix_.skip(XCODEC_SEGMENT_LENGTH); 
		soff = end;

		/*
		 * Now create the child slice.
		 */
		data_.push_back(slice);
	}

	/*
	 * The segment map should be empty, too.  It should only have entries
	 * that correspond to offset-hash entries.
	 */
	ASSERT(offset_seg_map.empty());
}

void
XCodecSlice::encode(XCBackref *backref, Buffer *output) const
{
	uint64_t lehash;

	if (!prefix_.empty()) {
		Buffer prefix(prefix_);
		prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(prefix);
	}

	std::set<uint64_t> need_declared(declarations_);

	std::vector<Data>::const_iterator dit;
	for (dit = data_.begin(); dit != data_.end(); ++dit) {
		Buffer prefix(dit->prefix_);
		prefix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(prefix);

		if (dit->seg_ == NULL)
			continue;

		if (need_declared.find(dit->hash_) != need_declared.end()) {
			uint64_t shash = XCHash<XCODEC_SEGMENT_LENGTH>::hash(dit->seg_->data());
			ASSERT(shash == dit->hash_);

			output->append(XCODEC_DECLARE_CHAR);
			lehash = LittleEndian::encode(dit->hash_);
			output->append((const uint8_t *)&lehash, sizeof lehash);
			output->append(dit->seg_);

			backref->declare(dit->hash_, dit->seg_);

			need_declared.erase(dit->hash_);
		}

		uint8_t b;
		if (backref->present(dit->hash_, &b)) {
			output->append(XCODEC_BACKREF_CHAR);
			output->append(b);
		} else {
			output->append(XCODEC_HASHREF_CHAR);
			lehash = LittleEndian::encode(dit->hash_);
			output->append((const uint8_t *)&lehash, sizeof lehash);

			backref->declare(dit->hash_, dit->seg_);
		}
	}

	ASSERT(need_declared.empty());

	if (!suffix_.empty()) {
		Buffer suffix(suffix_);
		suffix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(suffix);
	}
}
