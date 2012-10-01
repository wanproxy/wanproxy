#include <event/event_callback.h>

#include <io/channel.h>
#include <io/pipe/pipe.h>
#include <io/pipe/splice.h>

/*
 * A Splice passes data unidirectionally between StreamChannels across a Pipe.
 */

Splice::Splice(const LogHandle& log, StreamChannel *source, Pipe *pipe, StreamChannel *sink)
: log_(""),
  source_(source),
  pipe_(pipe),
  sink_(sink),
  callback_(NULL),
  callback_action_(NULL),
  read_eos_(false),
  read_action_(NULL),
  input_action_(NULL),
  output_eos_(false),
  output_action_(NULL),
  write_action_(NULL),
  shutdown_action_(NULL)
{
	log_ = log + "/splice";

	ASSERT(log_, source_ != NULL);
	ASSERT(log_, sink_ != NULL);
}

Splice::~Splice()
{
	ASSERT(log_, callback_ == NULL);
	ASSERT(log_, callback_action_ == NULL);
	ASSERT(log_, read_action_ == NULL);
	ASSERT(log_, input_action_ == NULL);
	ASSERT(log_, output_action_ == NULL);
	ASSERT(log_, write_action_ == NULL);
	ASSERT(log_, shutdown_action_ == NULL);
}

Action *
Splice::start(EventCallback *cb)
{
	ASSERT(log_, callback_ == NULL && callback_action_ == NULL);
	callback_ = cb;

	EventCallback *scb = callback(this, &Splice::read_complete);
	read_action_ = source_->read(0, scb);

	if (pipe_ != NULL) {
		EventCallback *pcb = callback(this, &Splice::output_complete);
		output_action_ = pipe_->output(pcb);
	}

	return (cancellation(this, &Splice::cancel));
}

void
Splice::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;

		ASSERT(log_, callback_action_ == NULL);

		if (read_action_ != NULL) {
			read_action_->cancel();
			read_action_ = NULL;
		}

		if (input_action_ != NULL) {
			input_action_->cancel();
			input_action_ = NULL;
		}

		if (output_action_ != NULL) {
			output_action_->cancel();
			output_action_ = NULL;
		}

		if (write_action_ != NULL) {
			write_action_->cancel();
			write_action_ = NULL;
		}

		if (shutdown_action_ != NULL) {
			shutdown_action_->cancel();
			shutdown_action_ = NULL;
		}
	} else {
		ASSERT(log_, callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;
	}
}

void
Splice::complete(Event e)
{
	ASSERT(log_, callback_ != NULL);
	ASSERT(log_, callback_action_ == NULL);

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}

	if (input_action_ != NULL) {
		input_action_->cancel();
		input_action_ = NULL;
	}

	if (output_action_ != NULL) {
		output_action_->cancel();
		output_action_ = NULL;
	}

	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	if (shutdown_action_ != NULL) {
		shutdown_action_->cancel();
		shutdown_action_ = NULL;
	}

	callback_->param(e);
	callback_action_ = callback_->schedule();
	callback_ = NULL;
}

void
Splice::read_complete(Event e)
{
	read_action_->cancel();
	read_action_ = NULL;

	ASSERT(log_, !read_eos_);

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (e.buffer_.empty()) {
		ASSERT(log_, e.type_ == Event::EOS);

		read_eos_ = true;
	}

	if (pipe_ != NULL) {
		ASSERT(log_, input_action_ == NULL);
		EventCallback *cb = callback(this, &Splice::input_complete);
		input_action_ = pipe_->input(&e.buffer_, cb);
	} else {
		if (e.type_ == Event::EOS && e.buffer_.empty()) {
			EventCallback *cb = callback(this, &Splice::shutdown_complete);
			shutdown_action_ = sink_->shutdown(false, true, cb);
			return;
		}

		ASSERT(log_, write_action_ == NULL);
		EventCallback *cb = callback(this, &Splice::write_complete);
		write_action_ = sink_->write(&e.buffer_, cb);
	}
}

void
Splice::input_complete(Event e)
{
	input_action_->cancel();
	input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	ASSERT(log_, read_action_ == NULL);
	if (!read_eos_) {
		EventCallback *cb = callback(this, &Splice::read_complete);
		read_action_ = source_->read(0, cb);
	} else if (output_eos_) {
		complete(Event::EOS);
	}
}

void
Splice::output_complete(Event e)
{
	output_action_->cancel();
	output_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (e.type_ == Event::EOS && e.buffer_.empty()) {
		EventCallback *cb = callback(this, &Splice::shutdown_complete);
		shutdown_action_ = sink_->shutdown(false, true, cb);
		return;
	}

	ASSERT(log_, write_action_ == NULL);
	EventCallback *cb = callback(this, &Splice::write_complete);
	write_action_ = sink_->write(&e.buffer_, cb);
}

void
Splice::write_complete(Event e)
{
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (pipe_ != NULL) {
		ASSERT(log_, output_action_ == NULL);
		EventCallback *cb = callback(this, &Splice::output_complete);
		output_action_ = pipe_->output(cb);
	} else {
		if (read_eos_) {
			if (shutdown_action_ == NULL) {
				EventCallback *cb = callback(this, &Splice::shutdown_complete);
				shutdown_action_ = sink_->shutdown(false, true, cb);
				return;
			}
		} else {
			EventCallback *cb = callback(this, &Splice::read_complete);
			read_action_ = source_->read(0, cb);
		}
	}
}

void
Splice::shutdown_complete(Event e)
{
	shutdown_action_->cancel();
	shutdown_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::Error:
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
		return;
	}

	if (e.type_ == Event::Error)
		DEBUG(log_) << "Could not shut down write channel.";
	if (read_eos_) {
		complete(Event::EOS);
	} else {
		output_eos_ = true;
	}
}
