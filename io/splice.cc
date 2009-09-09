#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/channel.h>
#include <io/pipe.h>
#include <io/splice.h>

/*
 * A Splice passes data unidirectionall between Channels across a Pipe.
 */

Splice::Splice(Channel *source, Pipe *pipe, Channel *sink)
: log_("/splice"),
  source_(source),
  pipe_(pipe),
  sink_(sink),
  callback_(NULL),
  callback_action_(NULL),
  read_action_(NULL),
  input_action_(NULL),
  output_action_(NULL),
  write_action_(NULL)
{
}

Splice::~Splice()
{
	ASSERT(callback_ == NULL);
	ASSERT(callback_action_ == NULL);
	ASSERT(read_action_ == NULL);
	ASSERT(input_action_ == NULL);
	ASSERT(output_action_ == NULL);
	ASSERT(write_action_ == NULL);
}

Action *
Splice::start(EventCallback *cb)
{
	ASSERT(callback_ == NULL && callback_action_ == NULL);
	callback_ = cb;

	EventCallback *scb = callback(this, &Splice::read_complete);
	read_action_ = source_->read(0, scb);

	EventCallback *pcb = callback(this, &Splice::output_complete);
	output_action_ = pipe_->output(pcb);

	return (cancellation(this, &Splice::cancel));
}

void
Splice::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;

		ASSERT(callback_action_ == NULL);

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
	} else {
		ASSERT(callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;
	}
}

void
Splice::complete(Event e)
{
	ASSERT(callback_ != NULL);
	ASSERT(callback_action_ == NULL);

	callback_->event(e);
	callback_action_ = EventSystem::instance()->schedule(callback_);
	callback_ = NULL;
}

void
Splice::read_complete(Event e)
{
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	ASSERT(input_action_ == NULL);
	EventCallback *cb = callback(this, &Splice::input_complete);
	input_action_ = pipe_->input(&e.buffer_, cb);
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
		ERROR(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	ASSERT(read_action_ == NULL);
	EventCallback *cb = callback(this, &Splice::read_complete);
	read_action_ = source_->read(0, cb);
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
		ERROR(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (e.type_ == Event::EOS && e.buffer_.empty()) {
		complete(e);
		return;
	}

	ASSERT(write_action_ == NULL);
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
		ERROR(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	ASSERT(output_action_ == NULL);
	EventCallback *cb = callback(this, &Splice::output_complete);
	output_action_ = pipe_->output(cb);
}
