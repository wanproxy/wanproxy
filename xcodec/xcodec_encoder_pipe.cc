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
}

XCodecEncoderPipe::~XCodecEncoderPipe()
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);
}

Action *
XCodecEncoderPipe::input(Buffer *buf, EventCallback *cb)
{
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
	} else {
		if (!buf->empty()) {
			encoder_.encode(&input_buffer_, buf);
			ASSERT(buf->empty());
		} else {
			input_eos_ = true;
		}
	}

	cb->event(Event(Event::Done, 0));
	return (EventSystem::instance()->schedule(cb));
}

Action *
XCodecEncoderPipe::output(EventCallback *cb)
{
	ASSERT(output_action_ == NULL);
	ASSERT(output_callback_ == NULL);

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

	output_callback_ = cb;

	return (cancellation(this, &XCodecEncoderPipe::output_cancel));
}

void
XCodecEncoderPipe::output_ready(void)
{
	if (input_eos_) {
		ERROR("/xcodec/encoder/pipe") << "Ignoring spontaneous output after EOS.";
		return;
	}

	if (output_callback_ != NULL) {
		ASSERT(output_action_ == NULL);

		Buffer empty;
		Buffer tmp;

		encoder_.encode(&tmp, &empty);
		ASSERT(!tmp.empty());

		output_callback_->event(Event(Event::Done, 0, tmp));
		output_action_ = EventSystem::instance()->schedule(output_callback_);
		output_callback_ = NULL;
	}
}

void
XCodecEncoderPipe::output_cancel(void)
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
