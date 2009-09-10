#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#include <io/pipe_null.h>
#include <io/socket.h>
#include <io/splice.h>
#include <io/splice_pair.h>

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
  incoming_splice_(NULL),
  outgoing_splice_(NULL),
  splice_pair_(NULL),
  splice_action_(NULL)
{
	EventCallback *cb = callback(this, &ProxyClient::connect_complete);
	remote_action_ = TCPClient::connect(&remote_socket_, family, remote_name, cb);
	if (remote_action_ == NULL) {
		ASSERT(remote_socket_ == NULL);

		ERROR(log_) << "Could not connect to: " << remote_name;

		EventCallback *lcb = callback(this, &ProxyClient::close_complete,
					      (void *)local_socket_);
		local_action_ = local_socket_->close(lcb);
		return;
	}

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
	ASSERT(incoming_splice_ == NULL);
	ASSERT(outgoing_splice_ == NULL);
	ASSERT(splice_pair_ == NULL);
	ASSERT(splice_action_ == NULL);
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

	outgoing_splice_ = new Splice(local_socket_, new PipeNull, remote_socket_);
	incoming_splice_ = new Splice(remote_socket_, new PipeNull, local_socket_);
	splice_pair_ = new SplicePair(outgoing_splice_, incoming_splice_);

	EventCallback *cb = callback(this, &ProxyClient::splice_complete);
	splice_action_ = splice_pair_->start(cb);
}

void
ProxyClient::splice_complete(Event e)
{
	splice_action_->cancel();
	splice_action_ = NULL;

	delete splice_pair_;
	splice_pair_ = NULL;

	delete outgoing_splice_;
	outgoing_splice_ = NULL;

	delete incoming_splice_;
	incoming_splice_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		break;
	}

	schedule_close();
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
	    splice_action_ == NULL) {
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

	if (splice_pair_ != NULL) {
		if (splice_action_ != NULL) {
			splice_action_->cancel();
			splice_action_ = NULL;
		}

		ASSERT(outgoing_splice_ != NULL);
		ASSERT(incoming_splice_ != NULL);

		delete splice_pair_;
		splice_pair_ = NULL;

		delete outgoing_splice_;
		outgoing_splice_ = NULL;

		delete incoming_splice_;
		incoming_splice_ = NULL;
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
