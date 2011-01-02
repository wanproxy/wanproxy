#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_null.h>
#include <io/pipe/pipe_pair.h>
#include <io/socket/socket.h>
#include <io/pipe/splice.h>
#include <io/pipe/splice_pair.h>

#include <io/net/tcp_client.h>

#include "proxy_connector.h"

ProxyConnector::ProxyConnector(const std::string& name,
			 PipePair *pipe_pair, Socket *local_socket,
			 SocketAddressFamily family,
			 const std::string& remote_name)
: log_("/wanproxy/proxy/" + name + "/connector"),
  stop_action_(NULL),
  local_action_(NULL),
  local_socket_(local_socket),
  remote_action_(NULL),
  remote_socket_(NULL),
  pipe_pair_(pipe_pair),
  incoming_pipe_(NULL),
  incoming_splice_(NULL),
  outgoing_pipe_(NULL),
  outgoing_splice_(NULL),
  splice_pair_(NULL),
  splice_action_(NULL)
{
	if (pipe_pair_ != NULL) {
		incoming_pipe_ = pipe_pair_->get_incoming();
		outgoing_pipe_ = pipe_pair_->get_outgoing();
	} else {
		incoming_pipe_ = new PipeNull();
		outgoing_pipe_ = new PipeNull();
	}

	EventCallback *cb = callback(this, &ProxyConnector::connect_complete);
	remote_action_ = TCPClient::connect(family, remote_name, cb);

	Callback *scb = callback(this, &ProxyConnector::stop);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
}

ProxyConnector::~ProxyConnector()
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

	if (pipe_pair_ != NULL) {
		delete pipe_pair_;
		pipe_pair_ = NULL;
	} else {
		if (incoming_pipe_ != NULL) {
			delete incoming_pipe_;
			incoming_pipe_ = NULL;
		}

		if (outgoing_pipe_ != NULL) {
			delete outgoing_pipe_;
			outgoing_pipe_ = NULL;
		}
	}
}

void
ProxyConnector::close_complete(Event e, void *channel)
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
ProxyConnector::connect_complete(Event e)
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

	remote_socket_ = (Socket *)e.data_;
	ASSERT(remote_socket_ != NULL);

	incoming_splice_ = new Splice(log_ + "/incoming", local_socket_, incoming_pipe_, remote_socket_);
	outgoing_splice_ = new Splice(log_ + "/outgoing", remote_socket_, outgoing_pipe_, local_socket_);

	splice_pair_ = new SplicePair(outgoing_splice_, incoming_splice_);

	EventCallback *cb = callback(this, &ProxyConnector::splice_complete);
	splice_action_ = splice_pair_->start(cb);
}

void
ProxyConnector::splice_complete(Event e)
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
ProxyConnector::stop(void)
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
ProxyConnector::schedule_close(void)
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
	EventCallback *lcb = callback(this, &ProxyConnector::close_complete,
				      (void *)local_socket_);
	local_action_ = local_socket_->close(lcb);

	ASSERT(remote_action_ == NULL);
	if (remote_socket_ != NULL) {
		EventCallback *rcb = callback(this, &ProxyConnector::close_complete,
					      (void *)remote_socket_);
		remote_action_ = remote_socket_->close(rcb);
	}
}
