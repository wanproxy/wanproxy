#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_encoder_pipe.h>

XCodecEncoderPipe::XCodecEncoderPipe(XCodec *codec)
: encoder_(codec, this),
  input_buffer_(),
  input_eos_(false),
  output_action_(NULL),
  output_callback_(NULL)
{
	ASSERT(output_callback_ == NULL || output_action_ == NULL);
}

XCodecEncoderPipe::~XCodecEncoderPipe()
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
XCodecEncoderPipe::input(Buffer *buf, EventCallback *cb)
{
	ASSERT(output_callback_ == NULL || output_action_ == NULL);
	if (output_callback_ != NULL) {
		ASSERT(input_buffer_.empty());
		ASSERT(output_action_ == NULL);

		if (!buf->empty()) {
			Buffer tmp;

			encoder_.encode(&tmp, buf);
			ASSERT(buf->empty());

			output_callback_->event(Event(Event::Done, 0, tmp));
		} else {
			input_eos_ = true;
			output_callback_->event(Event(Event::EOS, 0));
		}
		output_action_ = EventSystem::instance()->schedule(output_callback_);
		output_callback_ = NULL;
		ASSERT(output_callback_ == NULL || output_action_ == NULL);
	} else {
		if (!buf->empty()) {
			encoder_.encode(&input_buffer_, buf);
			ASSERT(buf->empty());
		} else {
			input_eos_ = true;
		}
	}
	ASSERT(output_callback_ == NULL || output_action_ == NULL);

	cb->event(Event(Event::Done, 0));
	return (EventSystem::instance()->schedule(cb));
}

Action *
XCodecEncoderPipe::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

	ASSERT(output_callback_ == NULL || output_action_ == NULL);

	if (!input_buffer_.empty() || input_eos_) {
		if (input_eos_) {
			cb->event(Event(Event::EOS, 0, input_buffer_));
			if (!input_buffer_.empty())
				input_buffer_.clear();
		} else {
			cb->event(Event(Event::Done, 0, input_buffer_));
			input_buffer_.clear();
		}

		return (EventSystem::instance()->schedule(cb));
	}

	ASSERT(output_callback_ == NULL || output_action_ == NULL);
	output_callback_ = cb;
	ASSERT(output_callback_ == NULL || output_action_ == NULL);

	return (cancellation(this, &XCodecEncoderPipe::output_cancel));
}

void
XCodecEncoderPipe::output_ready(void)
{
	if (input_eos_) {
		if (input_buffer_.empty()) {
			ERROR("/xcodec/encoder/pipe") << "Ignoring spontaneous output after EOS.";
			return;
		}
		DEBUG("/xcodec/encoder/pipe") << "Retrieving spontaneous data along with buffered data at EOS.";
	}

	Buffer empty;

	encoder_.encode(&input_buffer_, &empty);
	ASSERT(!input_buffer_.empty());

	if (output_callback_ != NULL) {
		/*
		 * If EOS has come, we can only get here if it is being serviced
		 * and there is data pending to be pushed before EOS.  Make sure
		 * that's the case.
		 */
		ASSERT(!input_eos_);

		ASSERT(output_callback_ == NULL || output_action_ == NULL);
		ASSERT(output_action_ == NULL);
		ASSERT(output_callback_ == NULL || output_action_ == NULL);

		output_callback_->event(Event(Event::Done, 0, input_buffer_));
		input_buffer_.clear();
		output_action_ = EventSystem::instance()->schedule(output_callback_);
		output_callback_ = NULL;
		ASSERT(output_callback_ == NULL || output_action_ == NULL);
	}
}

void
XCodecEncoderPipe::output_cancel(void)
{
	ASSERT(output_callback_ == NULL || output_action_ == NULL);
	if (output_action_ != NULL) {
		ASSERT(output_callback_ == NULL);

		output_action_->cancel();
		output_action_ = NULL;
	}

	ASSERT(output_callback_ == NULL || output_action_ == NULL);
	if (output_callback_ != NULL) {
		delete output_callback_;
		output_callback_ = NULL;
	}
	ASSERT(output_callback_ == NULL || output_action_ == NULL);
}
