/*
 * XXX
 *
 * We should just need <common/buffer.h>, but GCC is awful about
 * defining EventCallback as a tag after having a full definition
 * of it in a previous header file, so pipe.h can't do:
 * 	class EventCallback;
 * Unless no consumer is including <event/event_callback.h> before
 * pipe.h.
 */
#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_null.h>

/*
 * PipeNull is a pipe which passes data straight through and which provides no
 * discipline -- it will always immediately signal that it is ready to process
 * more data.
 */

PipeNull::PipeNull(void)
: PipeSimple("/io/pipe_null")
{
}

PipeNull::~PipeNull()
{ }

bool
PipeNull::process(Buffer *out, Buffer *in)
{
	if (!in->empty()) {
		in->moveout(out, in->length());
	}
	return (true);
}
