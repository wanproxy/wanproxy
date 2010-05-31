#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/splice.h>
#include <io/splice_pair.h>

/*
 * A SplicePair manages a pair of Splices.
 */

SplicePair::SplicePair(Splice *left, Splice *right)
: log_("/splice/pair"),
  left_(left),
  right_(right),
  callback_(NULL),
  callback_action_(NULL),
  left_action_(NULL),
  right_action_(NULL)
{
}

SplicePair::~SplicePair()
{
	ASSERT(callback_ == NULL);
	ASSERT(callback_action_ == NULL);
	ASSERT(left_action_ == NULL);
	ASSERT(right_action_ == NULL);
}

Action *
SplicePair::start(EventCallback *cb)
{
	ASSERT(callback_ == NULL && callback_action_ == NULL);
	callback_ = cb;

	EventCallback *lcb = callback(this, &SplicePair::splice_complete, left_);
	left_action_ = left_->start(lcb);

	EventCallback *rcb = callback(this, &SplicePair::splice_complete, right_);
	right_action_ = right_->start(rcb);

	return (cancellation(this, &SplicePair::cancel));
}

void
SplicePair::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;

		ASSERT(callback_action_ == NULL);

		if (left_action_ != NULL) {
			left_action_->cancel();
			left_action_ = NULL;
		}

		if (right_action_ != NULL) {
			right_action_->cancel();
			right_action_ = NULL;
		}
	} else {
		ASSERT(callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;
	}
}

void
SplicePair::splice_complete(Event e, Splice *splice)
{
	ASSERT(callback_ != NULL && callback_action_ == NULL);

	if (splice == left_) {
		left_action_->cancel();
		left_action_ = NULL;
	} else if (splice == right_) {
		right_action_->cancel();
		right_action_ = NULL;
	} else {
		NOTREACHED();
	}

	switch (e.type_) {
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		if (left_action_ != NULL) {
			left_action_->cancel();
			left_action_ = NULL;
		}
		if (right_action_ != NULL) {
			right_action_->cancel();
			right_action_ = NULL;
		}
		break;
	}

	if (left_action_ != NULL || right_action_ != NULL)
		return;

	ASSERT(e.buffer_.empty());
	if (e.type_ == Event::EOS) {
		callback_->param(Event::Done);
	} else {
		ASSERT(e.type_ != Event::Done);
		callback_->param(e);
	}
	callback_action_ = EventSystem::instance()->schedule(callback_);
	callback_ = NULL;
}
