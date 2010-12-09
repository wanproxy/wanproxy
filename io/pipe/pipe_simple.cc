#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
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
: log_(log),
  input_buffer_(),
  input_eos_(false),
  output_buffer_(),
  output_action_(NULL),
  output_callback_(NULL),
  process_error_(false)
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
	ASSERT(!process_error_);

	if (buf->empty()) {
		input_eos_ = true;
	} else {
		ASSERT(!input_eos_);
		input_buffer_.append(buf);
		buf->clear();
	}

	if (!process(&output_buffer_, &input_buffer_)) {
		process_error_ = true;

		input_buffer_.clear();
		output_buffer_.clear();
	}

	if (output_callback_ != NULL) {
		ASSERT(output_action_ == NULL);

		Action *a = output_do(output_callback_);
		if (a != NULL) {
			output_action_ = a;
			output_callback_ = NULL;
		}
	}

	if (process_error_)
		cb->param(Event::Error);
	else
		cb->param(Event::Done);
	return (cb->schedule());
}

Action *
PipeSimple::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	Action *a = output_do(cb);
	if (a != NULL)
		return (a);

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

Action *
PipeSimple::output_do(EventCallback *cb)
{
	if (process_error_) {
		ASSERT(output_buffer_.empty());

		cb->param(Event::Error);
		return (cb->schedule());
	}

	if (!output_buffer_.empty()) {
		cb->param(Event(Event::Done, output_buffer_));
		output_buffer_.clear();
		return (cb->schedule());
	}

	if (input_eos_) {
		cb->param(Event::EOS);
		return (cb->schedule());
	}

	return (NULL);
}
