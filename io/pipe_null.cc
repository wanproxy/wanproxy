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
: input_buffer_(),
  output_action_(NULL),
  output_callback_(NULL)
{
}

PipeNull::~PipeNull()
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
PipeNull::input(Buffer *buf, EventCallback *cb)
{
	ASSERT(!buf->empty()); /* XXX Support NULL/empty buf for flush/EOF?  */

	if (output_callback_ != NULL) {
		ASSERT(input_buffer_.empty());
		ASSERT(output_action_ == NULL);

		Buffer output;
		output.append(buf);
		buf->clear();

		output_callback_->event(Event(Event::Done, 0, output));
		output_action_ = EventSystem::instance()->schedule(output_callback_);
		output_callback_ = NULL;
	} else {
		input_buffer_.append(buf);
		buf->clear();
	}

	cb->event(Event(Event::Done, 0));
	return (EventSystem::instance()->schedule(cb));
}

Action *
PipeNull::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (!input_buffer_.empty()) {
		cb->event(Event(Event::Done, 0, input_buffer_));
		input_buffer_.clear();

		return (EventSystem::instance()->schedule(cb));
	}

	output_callback_ = cb;

	return (cancellation(this, &PipeNull::output_cancel));
}

void
PipeNull::output_cancel(void)
{
	if (output_action_ != NULL) {
		ASSERT(output_callback_ == NULL);

		output_action_->cancel();
		output_action_ = NULL;
	}

	if (output_callback_ != NULL) {
		delete output_callback_;
		output_callback_ = NULL;
	}
}
