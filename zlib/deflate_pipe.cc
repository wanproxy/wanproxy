#include <event/event_callback.h>

#include <io/pipe/pipe.h>

#include <zlib/deflate_pipe.h>

#define	DEFLATE_CHUNK_SIZE	65536

DeflatePipe::DeflatePipe(int level)
: PipeProducer("/zlib/deflate_pipe"),
  stream_()
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

void
DeflatePipe::consume(Buffer *in)
{
	Buffer out;
	uint8_t outbuf[DEFLATE_CHUNK_SIZE];
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
			int error = deflate(&stream_, flush);
			if (error == Z_OK && stream_.avail_out > 0 &&
			    flush == Z_NO_FLUSH)
				break;

			out.append(outbuf, sizeof outbuf - stream_.avail_out);
			stream_.avail_out = sizeof outbuf;
			stream_.next_out = outbuf;

			if (flush == Z_NO_FLUSH)
				break;
			if (flush == Z_SYNC_FLUSH && error == Z_OK) {
				if (!out.empty())
					produce(&out);
				return;
			}
			if (flush == Z_FINISH && error == Z_STREAM_END) {
				if (!out.empty())
					produce(&out);

				Buffer eos;
				produce(&eos);
				return;
			}
			/* More data to output.  */
		}

		if (seg != NULL)
			in->skip(seg->length() - stream_.avail_in);
	}
	NOTREACHED();
}
