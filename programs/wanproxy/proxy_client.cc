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
#include "proxy_peer.h"

ProxyClient::ProxyClient(XCodec *local_codec, XCodec *remote_codec,
			 Channel *local_channel, const std::string& remote_name,
			 unsigned remote_port)
: log_("/wanproxy/proxy_client"),
  action_(NULL),
  remote_codec_(remote_codec),
  local_peer_(NULL),
  remote_client_(NULL),
  remote_peer_(NULL)
{
	local_peer_ = new ProxyPeer(log_ + "/local", this, local_channel,
				    local_codec);
	/* XXX Move the local_peer_->start() to connect_complete?  */
	local_peer_->start();
	EventCallback *cb = callback(this, &ProxyClient::connect_complete);
	action_ = TCPClient::connect(&remote_client_, remote_name, remote_port,
				     cb);
}

ProxyClient::~ProxyClient()
{
	DEBUG(log_) << "Destroying client.";

	ASSERT(action_ == NULL);
	if (local_peer_ != NULL) {
		delete local_peer_;
		local_peer_ = NULL;
	}
	ASSERT(remote_client_ == NULL);
	if (remote_peer_ != NULL) {
		delete remote_peer_;
		remote_peer_ = NULL;
	}
}

void
ProxyClient::close_peer(ProxyPeer *self)
{
	if (self == local_peer_)
		local_peer_ = NULL;
	else
		remote_peer_ = NULL;

	if (local_peer_ == NULL && remote_peer_ == NULL)
		delete this;
}

ProxyPeer *
ProxyClient::get_peer(ProxyPeer *self)
{
	if (self == local_peer_)
		return (remote_peer_);
	if (self == remote_peer_)
		return (local_peer_);
	NOTREACHED();
}

void
ProxyClient::connect_complete(Event e, void *)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		local_peer_->schedule_close();
		return;
	}

	Channel *remote_channel = remote_client_;
	remote_client_ = NULL;
	remote_peer_ = new ProxyPeer(log_ + "/remote", this, remote_channel,
				     remote_codec_);

	/* XXX Maybe start the local_peer_ here?  */
	remote_peer_->start();
}
