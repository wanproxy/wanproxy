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

struct xcodec_slice {
	Buffer prefix_;
	uint64_t hash_;
	BufferSegment *seg_;

	xcodec_slice(void)
	: prefix_(),
	  hash_(),
	  seg_(NULL)
	{ }
};

XCodecSlice::XCodecSlice(XCDatabase *database, Buffer *input)
: log_("/xcodec/slice"),
  database_(database),
  type_(XCodecSlice::EscapeData),
  prefix_(),
  hash_(0),
  seg_(NULL),
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

XCodecSlice::XCodecSlice(XCDatabase *database, Buffer *prefix, uint64_t hash, BufferSegment *seg)
: log_("/xcodec/slice"),
  database_(database),
  type_(XCodecSlice::HashReference),
  prefix_(),
  hash_(hash),
  seg_(NULL),
  suffix_()
{
	if (prefix != NULL) {
		prefix_.append(prefix);
		prefix->clear();
	}

	if (seg != NULL) {
		seg->ref();
		seg_ = seg;
	}
}

XCodecSlice::~XCodecSlice()
{
	std::map<uint64_t, BufferSegment *>::iterator it;
	while ((it = declarations_.begin()) != declarations_.end()) {
		BufferSegment *seg = it->second;
		seg->unref();
		declarations_.erase(it);
	}

	if (seg_ != NULL) {
		seg_->unref();
		seg_ = NULL;
	}

	std::deque<XCodecSlice *>::iterator chit;
	while ((chit = children_.begin()) != children_.end()) {
		XCodecSlice *slice = *chit;
		delete slice;
		children_.erase(chit);
	}
}

/*
 * This takes a view of a data stream and turns it into a series of references to other
 * data, declarations of data to be referenced, and data that needs escaped.
 *
 * It's very simple and aggressive and performs worse than the original encoder as a
 * result.  The old encoder did a few important things differently.  First, it would start
 * declaring data as soon as we knew we hadn't found a hash for a given chunk of data.  What
 * I mean is that it would look at a segment and a segment but one worth of data until it found
 * either a match or something that didn't collide, and then it would use that.
 *
 * Secondly, as a result of that, it didn't need to do two database lookups, and those things
 * get expensive.
 *
 * However, we have a few places where we can do better.  First, we can emulate the aggressive
 * declaration (which is better for data and for speed) by looking at the first ohit and seeing
 * when we're a segment length away from it, and then scanning it then and there to find something
 * we can use.  Then we can just put it into the offset_seg_map and we can avoid using the hash_map
 * later on.
 *
 * Second, we don't need to use a map.  We need the data to be sorted, yes, but just by insertion.
 * Using a deque or vector should give much better performance for the offset_hash_map and the
 * offset_seg_map.  We can just put a pair in them and manage the sorting ourselves, algorithmically.
 *
 * Fourth, probably a lot of other things.
 */
void
XCodecSlice::process(Buffer *input)
{
	ASSERT(prefix_.empty());
	ASSERT(suffix_.empty());
	ASSERT(type_ == XCodecSlice::EscapeData);
	ASSERT(input->length() >= XCODEC_SEGMENT_LENGTH);

	XCHash<XCODEC_SEGMENT_LENGTH> xcodec_hash;
	std::map<unsigned, uint64_t> offset_hash_map;
	std::map<unsigned, BufferSegment *> offset_seg_map;
	unsigned o = 0;
	unsigned base = 0;

	while (input->length() >= XCODEC_SEGMENT_LENGTH) {
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
				 * identical to this chunk of data, then
				 * that's positively fantastic.
				 */
				uint8_t data[XCODEC_SEGMENT_LENGTH];
				suffix_.copyout(data, start, sizeof data);

				if (!oseg->match(data, sizeof data)) {
					DEBUG(log_) << "Collision in first pass.";
					continue;
				}

				/* Identical to this chunk.  */
				/* The offset-seg map holds our reference.  */
				offset_seg_map[start] = oseg;
				offset_hash_map[start] = hash;

				/* Do not hash any data until after us.  */
				base = o + XCODEC_SEGMENT_LENGTH;
			}

			/*
			 * No collision, remember this for later.
			 */
			offset_hash_map[start] = hash;
		}
		seg->unref();
	}

	/*
	 * Now compile the offset-hash map into child slices.
	 */
	std::map<unsigned, uint64_t>::iterator ohit;
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

		std::map<unsigned, BufferSegment *>::iterator osit = offset_seg_map.begin();
		xcodec_slice slice;

		/*
		 * If this offset-hash corresponds to this offset-segment, use it!
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
				 * This hash would overlap with a offset-segment.
				 * Skip it.
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
		 * We have not yet set a seg_ in this xcodec_slice, so it's time
		 * for us to declare this segment.
		 */
		if (slice.seg_ == NULL) {
			uint8_t data[XCODEC_SEGMENT_LENGTH];
			suffix_.copyout(data, start - soff, sizeof data);

			/*
			 * We can't assume that this isn't in the database.  Since
			 * we're declaring things all the time in this stream, we may
			 * have introduced hits and collisions.  So we, sadly, have
			 * to go back to the well.
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
				 * No hit is fantastic, too -- go ahead and declare this
				 * hash.
				 */
				seg = new BufferSegment();
				seg->append(data, sizeof data);

				database_->enter(hash, seg);

				/* Take a reference for the declarations_.  */
				/* XXX Make sure no dupes!  */
				seg->ref();
				declarations_[hash] = seg;

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
			 * There should not be any successive overlapping
			 * hashes if we found a hit in the first pass.  We
			 * should ASSERT that this looks like we'd expect.
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
		children_.push_back(new XCodecSlice(database_, &slice.prefix_, slice.hash_, slice.seg_));
		ASSERT(slice.prefix_.empty());

		/*
		 * Drop the reference we held.
		 */
		slice.seg_->unref();
		slice.seg_ = NULL;
	}

	/*
	 * The segment map should be empty, too.  It should only have entries that
	 * correspond to offset-hash entries.
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

	/*
	 * XXX
	 * This destroys locality of reference.  Better to make the first
	 * reference trigger declaration.  This gives massively worse overhead
	 * since the backref window is basically useless.
	 */
	std::map<uint64_t, BufferSegment *>::const_iterator dit;
	for (dit = declarations_.begin(); dit != declarations_.end(); ++dit) {
		uint64_t hash = dit->first;
		BufferSegment *seg = dit->second;

		output->append(XCODEC_DECLARE_CHAR);
		lehash = LittleEndian::encode(hash);
		output->append((const uint8_t *)&lehash, sizeof lehash);
		output->append(seg);

		backref->declare(hash, seg);
	}

	if (type_ == XCodecSlice::HashReference) {
		uint8_t b;

		if (backref->present(hash_, &b)) {
			output->append(XCODEC_BACKREF_CHAR);
			output->append(b);
		} else {
			output->append(XCODEC_HASHREF_CHAR);
			lehash = LittleEndian::encode(hash_);
			output->append((const uint8_t *)&lehash, sizeof lehash);

			backref->declare(hash_, seg_);
		}
	}

	std::deque<XCodecSlice *>::const_iterator chit;
	for (chit = children_.begin(); chit != children_.end(); ++chit) {
		const XCodecSlice *slice = *chit;

		slice->encode(backref, output);
	}

	if (!suffix_.empty()) {
		Buffer suffix(suffix_);
		suffix.escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
		output->append(suffix);
	}
}
