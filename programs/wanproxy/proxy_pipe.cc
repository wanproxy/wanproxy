#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/channel.h>
#include <io/file_descriptor.h>

#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>

#include "proxy_pipe.h"

ProxyPipe::ProxyPipe(XCodec *read_codec, Channel *read_channel,
		     Channel *write_channel, XCodec *write_codec)
: log_("/wanproxy/proxy/pipe"),
  read_action_(NULL),
  read_buffer_(),
  read_channel_(read_channel),
  read_decoder_(NULL),
  write_action_(NULL),
  write_buffer_(),
  write_channel_(write_channel),
  write_encoder_(NULL),
  flow_action_(NULL),
  flow_callback_(NULL)
{
	if (read_codec != NULL)
		read_decoder_ = read_codec->decoder();
	if (write_codec != NULL)
		write_encoder_ = write_codec->encoder();
}

ProxyPipe::~ProxyPipe()
{
	ASSERT(read_action_ == NULL);
	ASSERT(write_action_ == NULL);

	if (read_decoder_ != NULL) {
		delete read_decoder_;
		read_decoder_ = NULL;
	}

	if (write_encoder_ != NULL) {
		delete write_encoder_;
		write_encoder_ = NULL;
	}

	ASSERT(flow_action_ == NULL);
}

Action *
ProxyPipe::flow(EventCallback *cb)
{
	ASSERT(flow_callback_ == NULL);

	flow_callback_ = cb;
	schedule_read();
	return (cancellation(this, &ProxyPipe::flow_cancel));
}

void
ProxyPipe::read_complete(Event e, void *)
{
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		read_error();
		return;
	}

	read_buffer_.append(e.buffer_);
	if (!read_buffer_.empty()) {
		Buffer output;

		if (read_decoder_ != NULL) {
			if (!read_decoder_->decode(&output, &read_buffer_)) {
				read_error();
				return;
			}
		} else {
			output = read_buffer_;
			read_buffer_.clear();
		}

		if (write_encoder_ != NULL) {
			write_encoder_->encode(&write_buffer_, &output);
		} else {
			write_buffer_.append(output);
		}

		schedule_write();
	}

	if (e.type_ == Event::EOS) {
		read_channel_ = NULL;

		if (write_action_ == NULL) {
			flow_close();
		}
	} else {
		schedule_read();
	}
}

void
ProxyPipe::write_complete(Event e, void *)
{
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		write_error();
		return;
	}

	/*
	 * If the read channel is gone (due to EOS) and we've finished our
	 * writing, then tear down this flow.
	 */
	if (write_buffer_.empty() && read_channel_ == NULL) {
		flow_close();
	} else {
		schedule_write();
	}
}

void
ProxyPipe::schedule_read(void)
{
	ASSERT(read_action_ == NULL);
	ASSERT(read_channel_ != NULL);

	EventCallback *cb = callback(this, &ProxyPipe::read_complete);
	read_action_ = read_channel_->read(cb);
}

void
ProxyPipe::schedule_write(void)
{
	if (write_action_ != NULL)
		return;

	if (write_buffer_.empty())
		return;

	EventCallback *cb = callback(this, &ProxyPipe::write_complete);
	write_action_ = write_channel_->write(&write_buffer_, cb);
}

void
ProxyPipe::read_error(void)
{
	ASSERT(read_action_ == NULL);

	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	flow_error();
}

void
ProxyPipe::write_error(void)
{
	ASSERT(write_action_ == NULL);

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}

	flow_error();
}

void
ProxyPipe::flow_close(void)
{
	ASSERT(flow_action_ == NULL);
	ASSERT(flow_callback_ != NULL);

	flow_callback_->event(Event(Event::Done, 0));
	Action *a = EventSystem::instance()->schedule(flow_callback_);
	flow_action_ = a;
	flow_callback_ = NULL;
}

void
ProxyPipe::flow_error(void)
{
	ASSERT(flow_action_ == NULL);
	ASSERT(flow_callback_ != NULL);

	/* XXX Preserve error code.  */
	flow_callback_->event(Event(Event::Error, 0));
	Action *a = EventSystem::instance()->schedule(flow_callback_);
	flow_action_ = a;
	flow_callback_ = NULL;
}

void
ProxyPipe::flow_cancel(void)
{
	if (flow_action_ != NULL) {
		flow_action_->cancel();
		flow_action_ = NULL;
	}

	if (flow_callback_ != NULL) {
		delete flow_callback_;
		flow_callback_ = NULL;
	}
}
