#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_link.h>

/*
 * PipeLink is a pipe which passes data through two Pipes.
 */

PipeLink::PipeLink(Pipe *incoming_pipe, Pipe *outgoing_pipe)
: log_("/pipe/link"),
  incoming_pipe_(incoming_pipe),
  outgoing_pipe_(outgoing_pipe),
  input_action_(NULL),
  input_callback_(NULL)
{
}

PipeLink::~PipeLink()
{
	ASSERT(input_action_ == NULL);
	ASSERT(input_callback_ == NULL);
}

Action *
PipeLink::input(Buffer *buf, EventCallback *icb)
{
	ASSERT(input_action_ == NULL);
	ASSERT(input_callback_ == NULL);

	input_callback_ = icb;

	EventCallback *cb = callback(this, &PipeLink::incoming_input_complete);
	input_action_ = incoming_pipe_->input(buf, cb);

	return (cancellation(this, &PipeLink::input_cancel));
}

Action *
PipeLink::output(EventCallback *cb)
{
	return (outgoing_pipe_->output(cb));
}

void
PipeLink::input_cancel(void)
{
	if (input_action_ != NULL) {
		input_action_->cancel();
		input_action_ = NULL;
	}

	if (input_callback_ != NULL) {
		delete input_callback_;
		input_callback_ = NULL;
	}
}

void
PipeLink::input_complete(Event e)
{
	ASSERT(input_action_ == NULL);
	ASSERT(input_callback_ != NULL);

	input_callback_->param(e);
	input_action_ = input_callback_->schedule();
	input_callback_ = NULL;
}

void
PipeLink::incoming_input_complete(Event e)
{
	input_action_->cancel();
	input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		input_complete(e);
		return;
	}

	EventCallback *cb = callback(this, &PipeLink::incoming_output_complete);
	input_action_ = incoming_pipe_->output(cb);
}

void
PipeLink::incoming_output_complete(Event e)
{
	input_action_->cancel();
	input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		input_complete(e);
		return;
	}

	ASSERT(!e.buffer_.empty() || e.type_ == Event::EOS);

	EventCallback *cb = callback(this, &PipeLink::outgoing_input_complete);
	input_action_ = outgoing_pipe_->input(&e.buffer_, cb);
}

void
PipeLink::outgoing_input_complete(Event e)
{
	input_action_->cancel();
	input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		input_complete(e);
		return;
	}

	input_complete(e);
}
