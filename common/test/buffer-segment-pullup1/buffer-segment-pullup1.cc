#include <common/buffer.h>
#include <common/test.h>

int
main(void)
{
	TestGroup g("/test/buffer/prefix1", "BufferSegment::pullup #1");

	{
		BufferSegment *seg = new BufferSegment();
		seg->append("ABCD");
		{
			Test _(g, "BufferSegment match");
			if (seg->match("ABCD") && seg->length() == 4)
				_.pass();
		}
		{
			Test _(g, "BufferSegment skip");
			BufferSegment *seg2 = seg->skip(1);
			if (seg2 == seg && seg->match("BCD") &&
			    seg->length() == 3)
				_.pass();
		}
		{
			Test _(g, "BufferSegment append");
			BufferSegment *seg2 = seg->append('E');
			if (seg2 == seg && seg->match("BCDE") &&
			    seg->length() == 4)
				_.pass();
		}
		{
			Test _(g, "BufferSegment COW skip");
			seg->ref();
			BufferSegment *seg2 = seg->skip(1);
			if (seg2 != seg && seg2->match("CDE") &&
			    seg2->length() == 3 && !seg2->match(seg) &&
			    seg->match("BCDE") && seg->length() == 4)
				_.pass();
			seg2->unref();
		}
		{
			Test _(g, "BufferSegment COW append");
			seg->ref();
			BufferSegment *seg2 = seg->append('F');
			if (seg2 != seg && seg2->match("BCDEF") &&
			    seg2->length() == 5 && !seg2->match(seg) &&
			    seg->match("BCDE") && seg->length() == 4)
				_.pass();
			seg2->unref();
		}
		{
			Test _(g, "BufferSegment post-COW skip");
			BufferSegment *seg2 = seg->skip(2);
			if (seg2 == seg && seg->match("DE") &&
			    seg2->length() == 2)
				_.pass();
		}
		{
			Test _(g, "BufferSegment post-COW append");
			BufferSegment *seg2 = seg->append('F');
			if (seg2 == seg && seg->match("DEF") &&
			    seg2->length() == 3)
				_.pass();
		}
		{
			Test _(g, "BufferSegment append-to-full");
			while (seg->avail() > 0) {
				BufferSegment *seg2 = seg->append('X');
				ASSERT(seg == seg2);
			}
			_.pass();
		}
		{
			Test _(g, "BufferSegment leading check");
			if (seg->prefix("DEF") && seg->avail() == 0)
				_.pass();
		}
		{
			Test _(g, "BufferSegment skip leading + check");
			BufferSegment *seg2 = seg->skip(4);
			if (seg2 == seg) {
				const uint8_t *p;
				for (p = seg->data(); p < seg->end(); p++) {
					if (*p != 'X')
						break;
				}
				if (p == seg->end())
					_.pass();
			}
		}
		{
			Test _(g, "BufferSegment append-to-full 2");
			while (seg->avail() > 0) {
				BufferSegment *seg2 = seg->append('X');
				ASSERT(seg == seg2);
			}
			_.pass();
		}
		{
			Test _(g, "BufferSegment check full");
			const uint8_t *p;
			for (p = seg->data(); p < seg->end(); p++) {
				if (*p != 'X')
					break;
			}
			if (p == seg->end())
				_.pass();
		}
		seg->unref();
	}
}
