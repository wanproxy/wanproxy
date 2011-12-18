#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

/*
 * PipeProducer is a pipe with a producer-consume API.
 */

PipeProducer::PipeProducer(const LogHandle& log)
: log_(log),
  output_buffer_(),
  output_action_(NULL),
  output_callback_(NULL),
  output_eos_(false),
  error_(false)
{
}

PipeProducer::~PipeProducer()
{
	ASSERT(log_, output_action_ == NULL);
	ASSERT(log_, output_callback_ == NULL);
}

Action *
PipeProducer::input(Buffer *buf, EventCallback *cb)
{
	if (!error_) {
		/*
		 * XXX
		 * Allow consume() to only consume part of buf, and to
		 * delay further input.
		 */
		consume(buf);
		if (error_ && !buf->empty())
			buf->clear();
		ASSERT(log_, buf->empty());
	} else {
		buf->clear();
	}

	if (error_)
		cb->param(Event::Error);
	else
		cb->param(Event::Done);
	return (cb->schedule());
}

Action *
PipeProducer::output(EventCallback *cb)
{
	ASSERT(log_, output_action_ == NULL);
	ASSERT(log_, output_callback_ == NULL);

	Action *a = output_do(cb);
	if (a != NULL)
		return (a);

	output_callback_ = cb;

	return (cancellation(this, &PipeProducer::output_cancel));
}

void
PipeProducer::output_cancel(void)
{
	if (output_action_ != NULL) {
		ASSERT(log_, output_callback_ == NULL);

		output_action_->cancel();
		output_action_ = NULL;
	}

	if (output_callback_ != NULL) {
		delete output_callback_;
		output_callback_ = NULL;
	}
}

Action *
PipeProducer::output_do(EventCallback *cb)
{
	if (error_) {
		ASSERT(log_, output_buffer_.empty());

		cb->param(Event::Error);
		return (cb->schedule());
	}

	if (!output_buffer_.empty()) {
		cb->param(Event(Event::Done, output_buffer_));
		output_buffer_.clear();
		return (cb->schedule());
	}

	if (output_eos_) {
		cb->param(Event::EOS);
		return (cb->schedule());
	}

	return (NULL);
}

void
PipeProducer::produce(Buffer *buf)
{
	ASSERT(log_, !error_);
	ASSERT(log_, !output_eos_);

	if (!buf->empty()) {
		buf->moveout(&output_buffer_);
	} else {
		DEBUG(log_) << "Consider using produce_eos instead.";
		output_eos_ = true;
	}

	if (output_callback_ != NULL) {
		ASSERT(log_, output_action_ == NULL);

		Action *a = output_do(output_callback_);
		if (a != NULL) {
			output_action_ = a;
			output_callback_ = NULL;
		}
	}
}

void
PipeProducer::produce_eos(Buffer *buf)
{
	ASSERT(log_, !error_);
	ASSERT(log_, !output_eos_);

	if (buf != NULL && !buf->empty()) {
		buf->moveout(&output_buffer_);
	}
	output_eos_ = true;

	if (output_callback_ != NULL) {
		ASSERT(log_, output_action_ == NULL);

		Action *a = output_do(output_callback_);
		if (a != NULL) {
			output_action_ = a;
			output_callback_ = NULL;
		}
	}
}

void
PipeProducer::produce_error(void)
{
	ASSERT(log_, !error_);
	ASSERT(log_, !output_eos_);

	error_ = true;
	output_buffer_.clear();

	if (output_callback_ != NULL) {
		ASSERT(log_, output_action_ == NULL);

		Action *a = output_do(output_callback_);
		if (a != NULL) {
			output_action_ = a;
			output_callback_ = NULL;
		}
	}
}
