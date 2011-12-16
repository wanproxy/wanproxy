#include <event/event_callback.h>

#include <io/pipe/pipe.h>

#include <zlib/inflate_pipe.h>

#define	INFLATE_CHUNK_SIZE	65536

InflatePipe::InflatePipe(void)
: PipeProducer("/zlib/inflate_pipe"),
  stream_()
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
		ERROR(log_) << "Inflate stream did not end cleanly.";
}

void
InflatePipe::consume(Buffer *in)
{
	Buffer out;
	uint8_t outbuf[INFLATE_CHUNK_SIZE];
	bool first = true;

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
			    error == Z_MEM_ERROR) {
				ERROR(log_) << "inflate(): " << zError(error);
				produce_error();
				return;
			}

			if (flush != Z_NO_FLUSH && error == Z_BUF_ERROR &&
			    stream_.avail_out == sizeof outbuf)
				error = Z_OK;

			if (stream_.avail_out != sizeof outbuf)
				out.append(outbuf, sizeof outbuf - stream_.avail_out);
			stream_.avail_out = sizeof outbuf;
			stream_.next_out = outbuf;

			if (flush == Z_NO_FLUSH)
				break;
			if (error == Z_OK || error == Z_STREAM_END) {
				if (flush == Z_FINISH && error == Z_STREAM_END) {
					produce_eos(&out);
				} else {
					/*
					 * XXX
					 * How can it happen that we get nothing
					 * here?
					 */
					if (!out.empty())
						produce(&out);
				}

				if (!in->empty()) {
					ERROR(log_) << "Stream ended but more data follows.";
					produce_error();
				}
				return;
			}
			/* More data to output.  */
		}

		if (seg != NULL)
			in->skip(seg->length() - stream_.avail_in);
	}
	NOTREACHED();
}
