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
  input_eos_(false),
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
	if (output_callback_ != NULL) {
		ASSERT(input_buffer_.empty());
		ASSERT(output_action_ == NULL);

		if (!buf->empty()) {
			Buffer tmp;
			tmp.append(buf);
			buf->clear();

			output_callback_->event(Event(Event::Done, 0, tmp));
		} else {
			input_eos_ = true;
			output_callback_->event(Event(Event::EOS, 0));
		}
		output_action_ = EventSystem::instance()->schedule(output_callback_);
		output_callback_ = NULL;
	} else {
		if (!buf->empty()) {
			input_buffer_.append(buf);
			buf->clear();
		} else {
			input_eos_ = true;
		}
	}

	cb->event(Event(Event::Done, 0));
	return (EventSystem::instance()->schedule(cb));
}

Action *
PipeNull::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (!input_buffer_.empty() || input_eos_) {
		if (input_eos_) {
			cb->event(Event(Event::EOS, 0, input_buffer_));
			if (!input_buffer_.empty())
				input_buffer_.clear();
		} else {
			cb->event(Event(Event::Done, 0, input_buffer_));
			input_buffer_.clear();
		}

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
