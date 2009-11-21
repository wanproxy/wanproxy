#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#include <io/pipe_null.h>

/*
 * PipeNull is a pipe which passes data straight through and which provides no
 * discipline -- it will always immediately signal that it is ready to process
 * more data.
 */

PipeNull::PipeNull(void)
: PipeSimple()
{
}

PipeNull::~PipeNull()
{ }

void
PipeNull::process(Buffer *out, Buffer *in)
{
	if (!in->empty()) {
		in->moveout(out, in->length());
	}
}
