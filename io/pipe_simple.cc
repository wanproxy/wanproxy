#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#include <io/pipe_simple.h>

/*
 * PipeSimple is a pipe which passes data through a processing function and
 * which provides no discipline -- it will always immediately signal that it is
 * ready to process more data.
 *
 * TODO: Asynchronous processing function.
 *
 * XXX Should flag error and ensure all subsequent input() and output() fail.
 */

PipeSimple::PipeSimple(const LogHandle& log)
: log_(log),
  input_buffer_(),
  input_eos_(false),
  output_buffer_(),
  output_action_(NULL),
  output_callback_(NULL)
{
}

PipeSimple::~PipeSimple()
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
PipeSimple::input(Buffer *buf, EventCallback *cb)
{
	if (buf->empty()) {
		input_eos_ = true;
	} else {
		ASSERT(!input_eos_);
		input_buffer_.append(buf);
		buf->clear();
	}

	if (!process(&output_buffer_, &input_buffer_)) {
		if (output_callback_ != NULL) {
			ASSERT(output_action_ == NULL);

			input_buffer_.clear();
			output_buffer_.clear();

			output_callback_->param(Event::Error);
			output_action_ = EventSystem::instance()->schedule(output_callback_);
			output_callback_ = NULL;
		}

		cb->param(Event::Error);
		return (EventSystem::instance()->schedule(cb));
	}

	if (output_callback_ != NULL &&
	    (input_eos_ || !output_buffer_.empty())) {
		ASSERT(output_action_ == NULL);

		if (output_buffer_.empty())
			output_callback_->param(Event::EOS);
		else {
			output_callback_->param(Event(Event::Done, output_buffer_));
			output_buffer_.clear();
		}

		output_action_ = EventSystem::instance()->schedule(output_callback_);
		output_callback_ = NULL;
	}

	cb->param(Event::Done);
	return (EventSystem::instance()->schedule(cb));
}

Action *
PipeSimple::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (input_eos_ || !output_buffer_.empty()) {
		if (output_buffer_.empty())
			cb->param(Event::EOS);
		else {
			cb->param(Event(Event::Done, output_buffer_));
			output_buffer_.clear();
		}

		return (EventSystem::instance()->schedule(cb));
	}

	output_callback_ = cb;

	return (cancellation(this, &PipeSimple::output_cancel));
}

void
PipeSimple::output_cancel(void)
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
