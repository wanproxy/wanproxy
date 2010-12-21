#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_link.h>

/*
 * PipeLink is a pipe which passes data through two Pipes.
 */

PipeLink::PipeLink(Pipe *incoming_pipe, Pipe *outgoing_pipe)
: log_("/pipe/link"),
  incoming_pipe_(incoming_pipe),
  incoming_input_action_(NULL),
  incoming_output_eos_(false),
  incoming_output_action_(NULL),
  outgoing_pipe_(outgoing_pipe),
  outgoing_input_action_(NULL),
  outgoing_output_action_(NULL),
  input_action_(NULL),
  input_callback_(NULL),
  output_buffer_(),
  output_eos_(false),
  output_action_(NULL),
  output_callback_(NULL)
{
}

PipeLink::~PipeLink()
{
	ASSERT(incoming_input_action_ == NULL);

	if (incoming_output_action_ != NULL) {
		ASSERT(outgoing_input_action_ == NULL);

		incoming_output_action_->cancel();
		incoming_output_action_ = NULL;
	}

	if (outgoing_input_action_ != NULL) {
		ASSERT(incoming_output_action_ == NULL);

		outgoing_input_action_->cancel();
		outgoing_input_action_ = NULL;
	}

	ASSERT(outgoing_output_action_ == NULL);

	ASSERT(input_action_ == NULL);
	ASSERT(input_callback_ == NULL);

	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
PipeLink::input(Buffer *buf, EventCallback *icb)
{
	ASSERT(input_action_ == NULL);
	ASSERT(input_callback_ == NULL);

	ASSERT(incoming_input_action_ == NULL);

	EventCallback *cb = callback(this, &PipeLink::incoming_input_complete);
	incoming_input_action_ = incoming_pipe_->input(buf, cb);
	ASSERT(buf->empty());

	input_callback_ = icb;

	return (cancellation(this, &PipeLink::input_cancel));
}

Action *
PipeLink::output(EventCallback *ocb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (!output_buffer_.empty()) {
		ocb->param(Event(Event::Done, output_buffer_));
		output_buffer_.clear();
		return (ocb->schedule());
	}

	if (output_eos_) {
		ocb->param(Event::EOS);
		return (ocb->schedule());
	}

	if (incoming_output_action_ == NULL && outgoing_input_action_ == NULL) {
		if (!incoming_output_eos_) {
			EventCallback *cb = callback(this, &PipeLink::incoming_output_complete);
			incoming_output_action_ = incoming_pipe_->output(cb);
		}
	}

	ASSERT(outgoing_output_action_ == NULL);

	EventCallback *cb = callback(this, &PipeLink::outgoing_output_complete);
	outgoing_output_action_ = outgoing_pipe_->output(cb);

	output_callback_ = ocb;

	return (cancellation(this, &PipeLink::output_cancel));
}

void
PipeLink::input_cancel(void)
{
	if (input_action_ != NULL) {
		ASSERT(input_callback_ == NULL);
		ASSERT(incoming_input_action_ == NULL);

		input_action_->cancel();
		input_action_ = NULL;
	} else {
		if (incoming_input_action_ != NULL) {
			incoming_input_action_->cancel();
			incoming_input_action_ = NULL;
		}

		ASSERT(input_callback_ != NULL);
		delete input_callback_;
		input_callback_ = NULL;
	}
}

void
PipeLink::output_cancel(void)
{
	if (output_action_ != NULL) {
		ASSERT(output_callback_ == NULL);
		ASSERT(outgoing_output_action_ == NULL);

		output_action_->cancel();
		output_action_ = NULL;
	} else {
		if (outgoing_output_action_ != NULL) {
			outgoing_output_action_->cancel();
			outgoing_output_action_ = NULL;
		}

		ASSERT(output_callback_ != NULL);
		delete output_callback_;
		output_callback_ = NULL;
	}

	/* NB: Leave incoming:output->outgoing:input running.  */
}

void
PipeLink::incoming_input_complete(Event e)
{
	incoming_input_action_->cancel();
	incoming_input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::Error:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		/* XXX Error handling.  */
		return;
	}

	ASSERT(input_action_ == NULL);
	ASSERT(input_callback_ != NULL);

	input_callback_->param(e);
	input_action_ = input_callback_->schedule();
	input_callback_ = NULL;
}

void
PipeLink::incoming_output_complete(Event e)
{
	incoming_output_action_->cancel();
	incoming_output_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		/* XXX Error handling.  */
		return;
	}

	ASSERT(!incoming_output_eos_);
	if (e.type_ == Event::EOS)
		incoming_output_eos_ = true;

	ASSERT(outgoing_input_action_ == NULL);

	EventCallback *cb = callback(this, &PipeLink::outgoing_input_complete);
	outgoing_input_action_ = outgoing_pipe_->input(&e.buffer_, cb);
	ASSERT(e.buffer_.empty());
}

void
PipeLink::outgoing_input_complete(Event e)
{
	outgoing_input_action_->cancel();
	outgoing_input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		/* XXX Error handling.  */
		return;
	}

	ASSERT(incoming_output_action_ == NULL);

	if (!incoming_output_eos_ && output_callback_ != NULL) {
		EventCallback *cb = callback(this, &PipeLink::incoming_output_complete);
		incoming_output_action_ = incoming_pipe_->output(cb);
	}
}

void
PipeLink::outgoing_output_complete(Event e)
{
	outgoing_output_action_->cancel();
	outgoing_output_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		/* XXX Error handling.  */
		return;
	}

	if (!e.buffer_.empty()) {
		output_buffer_.append(e.buffer_);
		e.buffer_.clear();
	} else {
		ASSERT(e.type_ == Event::EOS);
	}

	if (e.type_ == Event::EOS)
		output_eos_ = true;

	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ != NULL);

	if (!output_buffer_.empty()) {
		output_callback_->param(Event(Event::Done, output_buffer_));
		output_buffer_.clear();
		output_action_ = output_callback_->schedule();
		output_callback_ = NULL;

		return;
	}

	ASSERT(output_eos_);

	output_callback_->param(Event::EOS);
	output_action_ = output_callback_->schedule();
	output_callback_ = NULL;
}
