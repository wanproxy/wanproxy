#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket.h>

#include <net/tcp_client.h>

#include "proxy_client.h"
#include "proxy_pipe.h"

ProxyClient::ProxyClient(XCodec *local_codec, XCodec *remote_codec,
			 Socket *local_socket, SocketAddressFamily family,
			 const std::string& remote_name)
: log_("/wanproxy/proxy/client"),
  stop_action_(NULL),
  local_action_(NULL),
  local_codec_(local_codec),
  local_socket_(local_socket),
  remote_action_(NULL),
  remote_codec_(remote_codec),
  remote_socket_(NULL),
  incoming_action_(NULL),
  incoming_pipe_(NULL),
  outgoing_action_(NULL),
  outgoing_pipe_(NULL)
{
	EventCallback *cb = callback(this, &ProxyClient::connect_complete);
	remote_action_ = TCPClient::connect(&remote_socket_, family, remote_name, cb);

	Callback *scb = callback(this, &ProxyClient::stop);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
}

ProxyClient::~ProxyClient()
{
	ASSERT(stop_action_ == NULL);
	ASSERT(local_action_ == NULL);
	ASSERT(local_socket_ == NULL);
	ASSERT(remote_action_ == NULL);
	ASSERT(remote_socket_ == NULL);
	ASSERT(incoming_action_ == NULL);
	ASSERT(incoming_pipe_ == NULL);
	ASSERT(outgoing_action_ == NULL);
	ASSERT(outgoing_pipe_ == NULL);
}

void
ProxyClient::close_complete(Event e, void *channel)
{
	if (channel == (void *)local_socket_) {
		local_action_->cancel();
		local_action_ = NULL;
	}

	if (channel == (void *)remote_socket_) {
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

	if (channel == (void *)local_socket_) {
		ASSERT(local_socket_ != NULL);
		delete local_socket_;
		local_socket_ = NULL;
	}

	if (channel == (void *)remote_socket_) {
		ASSERT(remote_socket_ != NULL);
		delete remote_socket_;
		remote_socket_ = NULL;
	}

	if (local_socket_ == NULL && remote_socket_ == NULL) {
		delete this;
	}
}

void
ProxyClient::connect_complete(Event e)
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

	outgoing_pipe_ = new ProxyPipe(local_codec_, local_socket_,
				       remote_socket_, remote_codec_);

	incoming_pipe_ = new ProxyPipe(remote_codec_, remote_socket_,
				       local_socket_, local_codec_);

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
ProxyClient::stop(void)
{
	stop_action_->cancel();
	stop_action_ = NULL;

	/*
	 * Connecting.
	 */
	if (local_action_ == NULL && remote_action_ != NULL &&
	    outgoing_pipe_ == NULL) {
		remote_action_->cancel();
		remote_action_ = NULL;

		schedule_close();
		return;
	}

	/*
	 * Already closing.  Should not happen.
	 */
	if (local_action_ != NULL || remote_action_ != NULL) {
		HALT(log_) << "Client already closing during stop.";
		return;
	}

	schedule_close();
}

void
ProxyClient::schedule_close(void)
{
	if (stop_action_ != NULL) {
		stop_action_->cancel();
		stop_action_ = NULL;
	}

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
	ASSERT(local_socket_ != NULL);
	EventCallback *lcb = callback(this, &ProxyClient::close_complete,
				      (void *)local_socket_);
	local_action_ = local_socket_->close(lcb);

	ASSERT(remote_action_ == NULL);
	ASSERT(remote_socket_ != NULL);
	EventCallback *rcb = callback(this, &ProxyClient::close_complete,
				      (void *)remote_socket_);
	remote_action_ = remote_socket_->close(rcb);
}
