#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#include <io/pipe_pair.h>
#include <io/pipe_pair_echo.h>

/*
 * PipePairEcho is a PipePair which provides an echo server to both halves of
 * the pair, with each pipe passing data to its peer / response pipe.  Like
 * PipeNull, the halves provides no limit on buffering, etc.
 */

PipePairEcho::Half::Half(Half *response_pipe)
: response_pipe_(response_pipe),
  output_action_(NULL),
  output_buffer_(),
  output_callback_(NULL)
{
}

PipePairEcho::Half::~Half()
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
PipePairEcho::Half::input(Buffer *buf, EventCallback *cb)
{
	ASSERT(!buf->empty()); /* XXX Support NULL/empty buf for flush/EOF?  */

	if (response_pipe_->output_callback_ != NULL) {
		ASSERT(response_pipe_->output_buffer_.empty());
		ASSERT(response_pipe_->output_action_ == NULL);

		Buffer tmp;
		tmp.append(buf);
		buf->clear();

		response_pipe_->output_callback_->event(Event(Event::Done, 0, tmp));
		response_pipe_->output_action_ = EventSystem::instance()->schedule(response_pipe_->output_callback_);
		response_pipe_->output_callback_ = NULL;
	} else {
		response_pipe_->output_buffer_.append(buf);
		buf->clear();
	}

	cb->event(Event(Event::Done, 0));
	return (EventSystem::instance()->schedule(cb));
}

Action *
PipePairEcho::Half::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (!output_buffer_.empty()) {
		cb->event(Event(Event::Done, 0, output_buffer_));
		output_buffer_.clear();

		return (EventSystem::instance()->schedule(cb));
	}

	output_callback_ = cb;

	return (cancellation(this, &PipePairEcho::Half::output_cancel));
}

void
PipePairEcho::Half::output_cancel(void)
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
