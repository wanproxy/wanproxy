#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_simple.h>

/*
 * PipeSimple is a pipe which passes data through a processing function and
 * which provides no discipline -- it will always immediately signal that it is
 * ready to process more data.
 *
 * TODO: Asynchronous processing function.
 */

PipeSimple::PipeSimple(const LogHandle& log)
: PipeProducer(log)
{ }

PipeSimple::~PipeSimple()
{ }

void
PipeSimple::consume(Buffer *buf)
{
	bool input_eos = buf->empty();
	Buffer out;
	if (!process(&out, buf)) {
		produce_error();
		return;
	}
	if (!out.empty())
		produce(&out);
	if (input_eos) {
		Buffer eos;
		produce(&eos);
	}
}
