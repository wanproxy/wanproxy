#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>

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
  source_eos_(false),
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
	if (response_pipe_->output_callback_ != NULL) {
		ASSERT(response_pipe_->output_buffer_.empty());
		ASSERT(response_pipe_->output_action_ == NULL);

		if (!buf->empty()) {
			Buffer tmp;
			tmp.append(buf);
			buf->clear();

			response_pipe_->output_callback_->param(Event(Event::Done, tmp));
		} else {
			response_pipe_->source_eos_ = true;
			response_pipe_->output_callback_->param(Event::EOS);
		}
		response_pipe_->output_action_ = response_pipe_->output_callback_->schedule();
		response_pipe_->output_callback_ = NULL;
	} else {
		if (!buf->empty()) {
			response_pipe_->output_buffer_.append(buf);
			buf->clear();
		} else {
			response_pipe_->source_eos_ = true;
		}
	}

	cb->param(Event::Done);
	return (cb->schedule());
}

Action *
PipePairEcho::Half::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	if (!output_buffer_.empty() || source_eos_) {
		if (source_eos_) {
			cb->param(Event(Event::EOS, output_buffer_));
			if (!output_buffer_.empty())
				output_buffer_.clear();
		} else {
			cb->param(Event(Event::Done, output_buffer_));
			output_buffer_.clear();
		}

		return (cb->schedule());
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
