#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/channel.h>
#include <io/file_descriptor.h>
#include <io/socket.h>
#include <io/tcp_client.h>

#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>

#include "proxy_client.h"
#include "proxy_pipe.h"

/*
 * XXX
 * Need to set up a Socket::Address class and all that, to avoid this
 * stupid redundancy.
 */

ProxyClient::ProxyClient(XCodec *local_codec, XCodec *remote_codec,
			 Channel *local_channel, const std::string& remote_name,
			 unsigned remote_port)
: log_("/wanproxy/proxy/client"),
  local_action_(NULL),
  local_codec_(local_codec),
  local_channel_(local_channel),
  remote_action_(NULL),
  remote_codec_(remote_codec),
  remote_client_(NULL),
  incoming_action_(NULL),
  incoming_pipe_(NULL),
  outgoing_action_(NULL),
  outgoing_pipe_(NULL)
{
	EventCallback *cb = callback(this, &ProxyClient::connect_complete);
	remote_action_ = TCPClient::connect(&remote_client_, remote_name,
					    remote_port, cb);
}

ProxyClient::ProxyClient(XCodec *local_codec, XCodec *remote_codec,
			 Channel *local_channel, uint32_t remote_ip,
			 uint16_t remote_port)
: log_("/wanproxy/proxy/client"),
  local_action_(NULL),
  local_codec_(local_codec),
  local_channel_(local_channel),
  remote_action_(NULL),
  remote_codec_(remote_codec),
  remote_client_(NULL),
  incoming_action_(NULL),
  incoming_pipe_(NULL),
  outgoing_action_(NULL),
  outgoing_pipe_(NULL)
{
	EventCallback *cb = callback(this, &ProxyClient::connect_complete);
	remote_action_ = TCPClient::connect(&remote_client_, remote_ip,
					    remote_port, cb);
}


ProxyClient::~ProxyClient()
{
	ASSERT(local_action_ == NULL);
	ASSERT(local_channel_ == NULL);
	ASSERT(remote_action_ == NULL);
	ASSERT(remote_client_ == NULL);
	ASSERT(incoming_action_ == NULL);
	ASSERT(incoming_pipe_ == NULL);
	ASSERT(outgoing_action_ == NULL);
	ASSERT(outgoing_pipe_ == NULL);
}

void
ProxyClient::close_complete(Event e, void *channel)
{
	if (channel == (void *)local_channel_) {
		local_action_->cancel();
		local_action_ = NULL;
	}

	if (channel == (void *)remote_client_) {
		remote_action_->cancel();
		remote_action_ = NULL;
	}

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		/* XXX Never sure what to do here.  */
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	if (channel == (void *)local_channel_) {
		ASSERT(local_channel_ != NULL);
		delete local_channel_;
		local_channel_ = NULL;
	}

	if (channel == (void *)remote_client_) {
		ASSERT(remote_client_ != NULL);
		delete remote_client_;
		remote_client_ = NULL;
	}

	if (local_channel_ == NULL && remote_client_ == NULL) {
		delete this;
	}
}

void
ProxyClient::connect_complete(Event e, void *)
{
	remote_action_->cancel();
	remote_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error:
		INFO(log_) << "Connect failed: " << e;
		schedule_close();
		return;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	outgoing_pipe_ = new ProxyPipe(local_codec_, local_channel_,
				       remote_client_, remote_codec_);

	incoming_pipe_ = new ProxyPipe(remote_codec_, remote_client_,
				       local_channel_, local_codec_);

	EventCallback *ocb = callback(this, &ProxyClient::flow_complete,
				      (void *)outgoing_pipe_);
	outgoing_action_ = outgoing_pipe_->flow(ocb);

	EventCallback *icb = callback(this, &ProxyClient::flow_complete,
				      (void *)incoming_pipe_);
	incoming_action_ = incoming_pipe_->flow(icb);
}

void
ProxyClient::flow_complete(Event e, void *pipe)
{
	if (pipe == (void *)outgoing_pipe_) {
		outgoing_action_->cancel();
		outgoing_action_ = NULL;

		ASSERT(outgoing_pipe_ != NULL);
		delete outgoing_pipe_;
		outgoing_pipe_ = NULL;
	}

	if (pipe == (void *)incoming_pipe_) {
		incoming_action_->cancel();
		incoming_action_ = NULL;

		ASSERT(incoming_pipe_ != NULL);
		delete incoming_pipe_;
		incoming_pipe_ = NULL;
	}

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error:
		schedule_close();
		return;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	if (outgoing_pipe_ == NULL && incoming_pipe_ == NULL) {
		schedule_close();
		return;
	}

	if (incoming_pipe_ != NULL) {
		incoming_pipe_->drain();
	}
	if (outgoing_pipe_ != NULL) {
		outgoing_pipe_->drain();
	}
}

void
ProxyClient::schedule_close(void)
{
	if (outgoing_pipe_ != NULL) {
		ASSERT(outgoing_action_ != NULL);
		outgoing_action_->cancel();
		outgoing_action_ = NULL;

		delete outgoing_pipe_;
		outgoing_pipe_ = NULL;
	}

	if (incoming_pipe_ != NULL) {
		ASSERT(incoming_action_ != NULL);
		incoming_action_->cancel();
		incoming_action_ = NULL;

		delete incoming_pipe_;
		incoming_pipe_ = NULL;
	}

	ASSERT(local_action_ == NULL);
	ASSERT(local_channel_ != NULL);
	EventCallback *lcb = callback(this, &ProxyClient::close_complete,
				      (void *)local_channel_);
	local_action_ = local_channel_->close(lcb);

	ASSERT(remote_action_ == NULL);
	ASSERT(remote_client_ != NULL);
	EventCallback *rcb = callback(this, &ProxyClient::close_complete,
				      (void *)remote_client_);
	remote_action_ = remote_client_->close(rcb);
}
