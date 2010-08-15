#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>

#include <zlib/deflate_pipe.h>

#define	DEFLATE_CHUNK_SIZE	65536

DeflatePipe::DeflatePipe(int level)
: PipeSimple("/zlib/deflate_pipe"),
  stream_(),
  finished_(false)
{
	stream_.zalloc = Z_NULL;
	stream_.zfree = Z_NULL;
	stream_.opaque = Z_NULL;

	int error = deflateInit(&stream_, level);
	if (error != Z_OK)
		HALT(log_) << "Could not initialize deflate stream.";
}

DeflatePipe::~DeflatePipe()
{
	int error = deflateEnd(&stream_);
	if (error != Z_OK)
		ERROR(log_) << "Deflate stream did not end cleanly.";
}

bool
DeflatePipe::process(Buffer *out, Buffer *in)
{
	uint8_t outbuf[DEFLATE_CHUNK_SIZE];
	bool first = true;

	if (finished_) {
		ASSERT(in->empty());
		return (true);
	}

	stream_.avail_out = sizeof outbuf;
	stream_.next_out = outbuf;

	for (;;) {
		Buffer::SegmentIterator iter = in->segments();
		const BufferSegment *seg;
		int flush;

		if (iter.end()) {
			seg = NULL;
			flush = first ? Z_FINISH : Z_SYNC_FLUSH;

			stream_.avail_in = 0;
			stream_.next_in = Z_NULL;
		} else {
			seg = *iter;
			flush = Z_NO_FLUSH;
			first = false;

			stream_.avail_in = seg->length();
			stream_.next_in = (Bytef *)(uintptr_t)seg->data();
		}

		for (;;) {
			int error = deflate(&stream_, flush);
			if (error == Z_OK && stream_.avail_out > 0 &&
			    flush == Z_NO_FLUSH)
				break;

			out->append(outbuf, sizeof outbuf - stream_.avail_out);
			stream_.avail_out = sizeof outbuf;
			stream_.next_out = outbuf;

			if (flush == Z_NO_FLUSH)
				break;
			if (flush == Z_SYNC_FLUSH && error == Z_OK)
				return (true);
			if (flush == Z_FINISH && error == Z_STREAM_END) {
				finished_ = true;
				return (true);
			}
			/* More data to output.  */
		}

		if (seg != NULL)
			in->skip(seg->length() - stream_.avail_in);
	}
	NOTREACHED();
}

bool
DeflatePipe::process_eos(void) const
{
	return (finished_);
}
