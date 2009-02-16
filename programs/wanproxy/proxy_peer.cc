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

#include "proxy_client.h"
#include "proxy_peer.h"

ProxyPeer::ProxyPeer(const LogHandle& log, ProxyClient *client,
		     Channel *channel, XCodec *codec)
: log_(log),
  client_(client),
  channel_(channel),
  codec_(codec),
  decoder_(NULL),
  encoder_(NULL),
  close_action_(NULL),
  read_action_(NULL),
  read_buffer_(),
  write_action_(NULL),
  write_buffer_(),
  closed_(false),
  error_(false)
{
	if (codec_ != NULL) {
		decoder_ = codec_->decoder();
		encoder_ = codec_->encoder();
	}
}

ProxyPeer::~ProxyPeer()
{
	DEBUG(log_) << "Destroying peer.";
	if (decoder_ != NULL) {
		delete decoder_;
		decoder_ = NULL;
	}
	if (encoder_ != NULL) {
		delete encoder_;
		encoder_ = NULL;
	}
	ASSERT(closed_);
	ASSERT(close_action_ == NULL);
	ASSERT(read_action_ == NULL);
	ASSERT(write_action_ == NULL);
}

void
ProxyPeer::start(void)
{
	schedule_read();

	/*
	 * Check if the peer has data for us to write right away.
	 */
	ProxyPeer *peer = client_->get_peer(this);
	if (peer != NULL)
		peer->schedule_write(this);
}

void
ProxyPeer::close_complete(Event e, void *)
{
	close_action_->cancel();
	close_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		error_ = true;
		break;
	}

	ASSERT(!closed_);
	closed_ = true;
	ASSERT(channel_ != NULL);
	delete channel_;
	channel_ = NULL;

	INFO(log_) << "closed.";

	if (error_) {
		ProxyPeer *peer = client_->get_peer(this);
		DEBUG(log_) << "Closing peer in: " << __PRETTY_FUNCTION__;
		peer->schedule_close();
	}
	client_->close_peer(this);
}

void
ProxyPeer::read_complete(Event e, void *)
{
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::EOS:
		INFO(log_) << "End of stream.";
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		error_ = true;
		schedule_close();
		return;
	}

	read_buffer_.append(e.buffer_);
	e.buffer_.clear();

	ProxyPeer *peer = client_->get_peer(this);
	schedule_write(peer);

	if (e.type_ == Event::EOS) {
		schedule_close();
	} else {
		schedule_read();
	}
}

void
ProxyPeer::write_complete(Event e, void *)
{
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		error_ = true;
		schedule_close();
		return;
	}

	schedule_write();
}

void
ProxyPeer::schedule_close(void)
{
	if (close_action_ != NULL)
		return;

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}

	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	EventCallback *cb = callback(this, &ProxyPeer::close_complete);
	close_action_ = channel_->close(cb);

	/*
	 * Since we can no longer write data to our channel that is read from
	 * our peer, we should do a few things:
	 *   1. if our peer has a pending read, cancel it.
	 *   2. if our peer has no pending write, then close our peer, since it
	 *      can do nothing more.
	 */
	ProxyPeer *peer = client_->get_peer(this);
	if (peer != NULL) {
		if (peer->read_action_ != NULL) {
			peer->read_action_->cancel();
			peer->read_action_ = NULL;
		}
		if (peer->write_action_ == NULL) {
			DEBUG(log_) << "Closing peer in: " << __PRETTY_FUNCTION__;
			peer->schedule_close();
		}
	}
}

void
ProxyPeer::schedule_read(void)
{
	/*
	 * If the peer has closed or seen an error, and we are about to keep
	 * this socket open with a read even though there is no data to write,
	 * avoid the read and start a close.  There's nothing that could happen
	 * that would be good.
	 */
	ProxyPeer *peer = client_->get_peer(this);
	if (write_buffer_.empty() && write_action_ == NULL && peer != NULL &&
	    (peer->error_ || peer->closed_)) {
		DEBUG(log_) << "Closing self in: " << __PRETTY_FUNCTION__;
		schedule_close();
		return;
	}

	ASSERT(read_action_ == NULL);
	EventCallback *cb = callback(this, &ProxyPeer::read_complete);
	read_action_ = channel_->read(cb);
}

void
ProxyPeer::schedule_write(void)
{
	if (write_action_ != NULL)
		return;
	if (write_buffer_.empty()) {
		ProxyPeer *peer = client_->get_peer(this);
		if (peer == NULL || peer->closed_ || peer->error_) {
			DEBUG(log_) << "Closing self in: " << __PRETTY_FUNCTION__;
			schedule_close();
		}
		return;
	}
	EventCallback *cb = callback(this, &ProxyPeer::write_complete);
	write_action_ = channel_->write(&write_buffer_, cb);
}

void
ProxyPeer::schedule_write(Buffer *data)
{
	if (data != NULL) {
		if (encoder_ != NULL)
			encoder_->encode(&write_buffer_, data);
		else {
			write_buffer_.append(data);
			data->clear();
		}
	}

	schedule_write();
}

void
ProxyPeer::schedule_write(ProxyPeer *peer)
{
	if (peer == NULL)
		return;

	/*
	 * If we are attemtping to write to a closed peer, close our socket.
	 */
	if (peer->error_ || peer->closed_) {
		DEBUG(log_) << "Closing self in: " << __PRETTY_FUNCTION__;
		schedule_close();
		return;
	}

	if (!read_buffer_.empty()) {
		if (decoder_ != NULL) {
			Buffer output;
			decoder_->decode(&output, &read_buffer_);
			peer->schedule_write(&output);
		} else {
			peer->schedule_write(&read_buffer_);
		}
	}
}
