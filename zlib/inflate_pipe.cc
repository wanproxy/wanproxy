#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>

#include <zlib/inflate_pipe.h>

#define	INFLATE_CHUNK_SIZE	65536

InflatePipe::InflatePipe(void)
: PipeSimple("/zlib/inflate_pipe"),
  stream_(),
  finished_(false)
{
	stream_.zalloc = Z_NULL;
	stream_.zfree = Z_NULL;
	stream_.opaque = Z_NULL;

	stream_.avail_in = 0;
	stream_.next_in = Z_NULL;

	int error = inflateInit(&stream_);
	if (error != Z_OK)
		HALT(log_) << "Could not initialize inflate stream.";
}

InflatePipe::~InflatePipe()
{
	int error = inflateEnd(&stream_);
	if (error != Z_OK)
		HALT(log_) << "Could not end inflate stream.";
}

bool
InflatePipe::process(Buffer *out, Buffer *in)
{
	uint8_t outbuf[INFLATE_CHUNK_SIZE];
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
			int error = inflate(&stream_, flush);
			if (error == Z_OK && stream_.avail_out > 0 &&
			    flush == Z_NO_FLUSH)
				break;
			if (error == Z_NEED_DICT || error == Z_DATA_ERROR ||
			    error == Z_MEM_ERROR)
				return (false);

			if (flush == Z_SYNC_FLUSH && error == Z_BUF_ERROR &&
			    stream_.avail_out == sizeof outbuf)
				error = Z_OK;

			out->append(outbuf, sizeof outbuf - stream_.avail_out);
			stream_.avail_out = sizeof outbuf;
			stream_.next_out = outbuf;

			if (flush == Z_NO_FLUSH)
				break;
			if (flush == Z_SYNC_FLUSH && error == Z_OK)
				return (true);
			if (error == Z_STREAM_END) {
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
InflatePipe::process_eos(void) const
{
	return (finished_);
}
