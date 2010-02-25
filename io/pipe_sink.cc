#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#include <io/pipe_sink.h>

/*
 * PipeSink is a pipe which only passes through EOS.
 */

PipeSink::PipeSink(void)
: input_eos_(false),
  output_action_(NULL),
  output_callback_(NULL)
{
}

PipeSink::~PipeSink()
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
PipeSink::input(Buffer *buf, EventCallback *cb)
{
	if (buf->empty()) {
		input_eos_ = true;
		if (output_callback_ != NULL) {
			ASSERT(output_action_ == NULL);

			output_callback_->event(Event::EOS);

			output_action_ = EventSystem::instance()->schedule(output_callback_);
			output_callback_ = NULL;
		}
	}

	cb->event(Event::Done);
	return (EventSystem::instance()->schedule(cb));
}

Action *
PipeSink::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (input_eos_) {
		cb->event(Event::EOS);

		return (EventSystem::instance()->schedule(cb));
	}

	output_callback_ = cb;

	return (cancellation(this, &PipeSink::output_cancel));
}

void
PipeSink::output_cancel(void)
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
