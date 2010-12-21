#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_splice.h>

/*
 * PipeSplice feeds the output from one Pipe to the input of another.
 */

PipeSplice::PipeSplice(Pipe *source, Pipe *sink)
: log_("/io/pipe/splice"),
  source_(source),
  sink_(sink),
  source_eos_(false),
  action_(NULL),
  callback_(NULL)
{ }

PipeSplice::~PipeSplice()
{
	ASSERT(action_ == NULL);
	ASSERT(callback_ == NULL);
}

Action *
PipeSplice::start(EventCallback *scb)
{
	ASSERT(action_ == NULL);
	ASSERT(callback_ == NULL);

	EventCallback *cb = callback(this, &PipeSplice::output_complete);
	action_ = source_->output(cb);

	callback_ = scb;

	return (cancellation(this, &PipeSplice::cancel));
}

void
PipeSplice::cancel(void)
{
	if (action_ != NULL) {
		action_->cancel();
		action_ = NULL;
	}

	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;
	}
}

void
PipeSplice::output_complete(Event e)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
	case Event::Error:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	ASSERT(!source_eos_);

	if (e.type_ == Event::Error) {
		callback_->param(e);
		action_ = callback_->schedule();
		callback_ = NULL;

		return;
	}

	if (e.type_ == Event::EOS) {
		ASSERT(e.buffer_.empty());
		source_eos_ = true;
	} else {
		ASSERT(!e.buffer_.empty());
	}

	EventCallback *cb = callback(this, &PipeSplice::input_complete);
	action_ = sink_->input(&e.buffer_, cb);
}

void
PipeSplice::input_complete(Event e)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::Error:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	if (e.type_ == Event::Error) {
		callback_->param(e);
		action_ = callback_->schedule();
		callback_ = NULL;

		return;
	}

	if (source_eos_) {
		callback_->param(Event::EOS);
		action_ = callback_->schedule();
		callback_ = NULL;

		return;
	}

	EventCallback *cb = callback(this, &PipeSplice::output_complete);
	action_ = source_->output(cb);
}
