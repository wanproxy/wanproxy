/*
 * Copyright (c) 2011-2014 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <crypto/crypto_encryption.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <io/net/tcp_server.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include <io/socket/simple_server.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_server_host_key.h>
#include <ssh/ssh_session.h>
#include <ssh/ssh_transport_pipe.h>

/*
 * XXX Create an SSH chat program.  Let only one of each user be connected at a time.  Send lines of data to all others.
 */

class SSHConnection {
	struct SSHChannel {
		enum Mode {
			SetupMode,
			ShellMode,
			ExecMode,
			SubsystemMode
		};

		Mode mode_;

		uint32_t local_channel_;
		uint32_t local_window_size_;
		uint32_t local_packet_size_;
		uint32_t remote_channel_;
		uint32_t remote_window_size_;
		uint32_t remote_packet_size_;
		Buffer remote_window_;

		std::map<std::string, Buffer> environment_;

		SSHChannel(uint32_t local_channel, uint32_t remote_channel, uint32_t local_window_size, uint32_t local_packet_size, uint32_t remote_window_size, uint32_t remote_packet_size)
		: mode_(SetupMode),
		  local_channel_(local_channel),
		  local_window_size_(local_window_size),
		  local_packet_size_(local_packet_size),
		  remote_channel_(remote_channel),
		  remote_window_size_(remote_window_size),
		  remote_packet_size_(remote_packet_size),
		  remote_window_(),
		  environment_()
		{ }
	};

	LogHandle log_;
	Socket *peer_;
	SSH::Session session_;
	SSH::TransportPipe *pipe_;
	Action *receive_action_;
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
	uint32_t channel_next_;
	std::map<uint32_t, SSHChannel *> channel_map_;
public:
	SSHConnection(Socket *peer)
	: log_("/ssh/connection"),
	  peer_(peer),
	  session_(SSH::ServerRole),
	  pipe_(NULL),
	  splice_(NULL),
	  splice_action_(NULL),
	  close_action_(NULL),
	  channel_next_(0),
	  channel_map_()
	{
		session_.algorithm_negotiation_ = new SSH::AlgorithmNegotiation(&session_);
		if (session_.role_ == SSH::ServerRole) {
			SSH::ServerHostKey *server_host_key = SSH::ServerHostKey::server(session_.role_, "ssh-server1.pem");
			session_.algorithm_negotiation_->add_algorithm(server_host_key);
		}
		session_.algorithm_negotiation_->add_algorithms();

		pipe_ = new SSH::TransportPipe(&session_);
		EventCallback *rcb = callback(this, &SSHConnection::receive_complete);
		receive_action_ = pipe_->receive(rcb);

		splice_ = new Splice(log_ + "/splice", peer_, pipe_, peer_);
		EventCallback *cb = callback(this, &SSHConnection::splice_complete);
		splice_action_ = splice_->start(cb);
	}

	~SSHConnection()
	{
		ASSERT(log_, close_action_ == NULL);
		ASSERT(log_, splice_action_ == NULL);
		ASSERT(log_, splice_ == NULL);
		ASSERT(log_, receive_action_ == NULL);
		ASSERT(log_, pipe_ == NULL);
		ASSERT(log_, peer_ == NULL);
	}

private:
	void receive_complete(Event e)
	{
		receive_action_->cancel();
		receive_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			ERROR(log_) << "Unexpected event while waiting for a packet: " << e;
			return;
		}

		ASSERT(log_, !e.buffer_.empty());

		Buffer service;
		Buffer type;
		Buffer data;
		Buffer msg;
		SSHChannel *channel;
		uint32_t recipient_channel, sender_channel, window_size, packet_size;
		bool want_reply;
		std::map<uint32_t, SSHChannel *>::const_iterator chit;
		switch (e.buffer_.peek()) {
		case SSH::Message::TransportDisconnectMessage:
			break;
		case SSH::Message::TransportServiceRequestMessage:
			/* Claim to support any kind of service the client requests.  */
			e.buffer_.skip(1);
			if (e.buffer_.empty()) {
				ERROR(log_) << "No service name after transport request.";
				return;
			}
			if (!SSH::String::decode(&service, &e.buffer_)) {
				ERROR(log_) << "Could not decode service name.";
				return;
			}
			if (!e.buffer_.empty()) {
				ERROR(log_) << "Extraneous data after service name.";
				return;
			}

			msg.append(SSH::Message::TransportServiceAcceptMessage);
			SSH::String::encode(&msg, service);
			pipe_->send(&msg);

			if (service.equal("ssh-userauth")) {
				msg.append(SSH::Message::UserAuthenticationBannerMessage);
				SSH::String::encode(&msg, std::string(" *\r\n\007 * This is a test server.  Sessions, including authentication, may be logged.\r\n *\r\n"));
				SSH::String::encode(&msg, std::string("en-CA"));
				pipe_->send(&msg);
			}
			break;
		case SSH::Message::UserAuthenticationRequestMessage:
			/* Any authentication request succeeds.  */
			e.buffer_.skip(1);
			msg.append(SSH::Message::UserAuthenticationSuccessMessage);
			pipe_->send(&msg);
			break;
		case SSH::Message::ConnectionChannelOpen:
			/* Opening a channel.  */
			e.buffer_.skip(1);
			if (e.buffer_.empty()) {
				ERROR(log_) << "No channel type after channel open.";
				return;
			}
			if (!SSH::String::decode(&type, &e.buffer_)) {
				ERROR(log_) << "Could not decode channel type.";
				return;
			}
			if (!SSH::UInt32::decode(&sender_channel, &e.buffer_)) {
				ERROR(log_) << "Could not decode sender channel.";
				return;
			}
			if (!SSH::UInt32::decode(&window_size, &e.buffer_)) {
				ERROR(log_) << "Could not decode window size.";
				return;
			}
			if (!SSH::UInt32::decode(&packet_size, &e.buffer_)) {
				ERROR(log_) << "Could not decode packet size.";
				return;
			}

			/* Only support session channels.  */
			if (!type.equal("session")) {
				msg.append(SSH::Message::ConnectionChannelOpenFailure);
				SSH::UInt32::encode(&msg, sender_channel);
				SSH::UInt32::encode(&msg, 3);
				SSH::String::encode(&msg, std::string("Unsupported session type."));
				SSH::String::encode(&msg, std::string("en-CA"));
				pipe_->send(&msg);
				return;
			}

			recipient_channel = sender_channel;
			sender_channel = channel_setup(recipient_channel, window_size, packet_size);

			/* Set up session.  */
			msg.append(SSH::Message::ConnectionChannelOpenConfirmation);
			SSH::UInt32::encode(&msg, recipient_channel);
			SSH::UInt32::encode(&msg, sender_channel);
			SSH::UInt32::encode(&msg, window_size);
			SSH::UInt32::encode(&msg, packet_size);
			pipe_->send(&msg);
			break;
		case SSH::Message::ConnectionChannelRequest:
			/* For now just fail any channel request.  */
			e.buffer_.skip(1);
			if (e.buffer_.empty()) {
				ERROR(log_) << "No channel after channel request.";
				return;
			}
			if (!SSH::UInt32::decode(&recipient_channel, &e.buffer_)) {
				ERROR(log_) << "Could not decode recipient channel.";
				return;
			}
			if (!SSH::String::decode(&type, &e.buffer_)) {
				ERROR(log_) << "Could not decode channel request type.";
				return;
			}
			if (e.buffer_.empty()) {
				ERROR(log_) << "Missing want_reply field.";
				return;
			}
			want_reply = e.buffer_.pop();

			chit = channel_map_.find(recipient_channel);
			if (chit == channel_map_.end()) {
				if (want_reply) {
					msg.append(SSH::Message::ConnectionChannelRequestFailure);
					SSH::UInt32::encode(&msg, 0); /* XXX What to do for a request that fails due to unknown channel?  */
					pipe_->send(&msg);
				}
				break;
			}
			channel = chit->second;

			if (type.equal("pty-req")) {
				/* Fail for now.  */
			} else if (type.equal("env")) {
				Buffer keybuf;
				if (!SSH::String::decode(&keybuf, &e.buffer_)) {
					ERROR(log_) << "Could not decode environment key.";
					return;
				}

				Buffer value;
				if (!SSH::String::decode(&value, &e.buffer_)) {
					ERROR(log_) << "Could not decode environment value.";
					return;
				}

				std::string key;
				keybuf.extract(key);

				if (channel->environment_.find(key) == channel->environment_.end()) {
					channel->environment_[key] = value;

					DEBUG(log_) << "Client set environment variable.";
					if (want_reply) {
						msg.append(SSH::Message::ConnectionChannelRequestSuccess);
						SSH::UInt32::encode(&msg, channel->remote_channel_);
						pipe_->send(&msg);
					}
					break;
				}
				INFO(log_) << "Client attempted to set environment variable twice.";
				if (want_reply) {
					msg.append(SSH::Message::ConnectionChannelRequestFailure);
					SSH::UInt32::encode(&msg, channel->remote_channel_);
					pipe_->send(&msg);
				}
				break;
			} else if (type.equal("shell")) {
				if (channel->mode_ == SSHChannel::SetupMode) {
					channel->mode_ = SSHChannel::ShellMode;
					if (want_reply) {
						msg.append(SSH::Message::ConnectionChannelRequestSuccess);
						SSH::UInt32::encode(&msg, channel->remote_channel_);
						pipe_->send(&msg);
					}

					msg.append(SSH::Message::ConnectionChannelData);
					SSH::UInt32::encode(&msg, channel->remote_channel_);
					SSH::String::encode(&msg, std::string(">>> Connected to shell.\r\n"));
					pipe_->send(&msg);
					break;
				}
				ERROR(log_) << "Client requested a shell session, but session is not in setup mode.";
			} else {
				DEBUG(log_) << "Unhandled channel request type:" << std::endl << type.hexdump();
			}

			if (want_reply) {
				msg.append(SSH::Message::ConnectionChannelRequestFailure);
				SSH::UInt32::encode(&msg, channel->remote_channel_);
				pipe_->send(&msg);
			}
			break;
		case SSH::Message::ConnectionChannelWindowAdjust:
			/* Follow our peer's lead on window adjustments.  */
			break;
		case SSH::Message::ConnectionChannelData:
			/* For now, just ignore data.  Will pass to something shell-like eventually.  */
			e.buffer_.skip(1);
			if (e.buffer_.empty()) {
				ERROR(log_) << "No channel after channel data.";
				return;
			}
			if (!SSH::UInt32::decode(&recipient_channel, &e.buffer_)) {
				ERROR(log_) << "Could not decode recipient channel.";
				return;
			}
			if (!SSH::String::decode(&data, &e.buffer_)) {
				ERROR(log_) << "Could not decode channel data.";
				return;
			}
			if (!e.buffer_.empty()) {
				ERROR(log_) << "Extraneous data after channel data.";
				return;
			}

			/* Just ignore empty data.  */
			if (data.empty())
				break;

			chit = channel_map_.find(recipient_channel);
			if (chit == channel_map_.end()) {
				/* XXX Send some kind of error.  */
				break;
			}
			channel = chit->second;
			data.moveout(&channel->remote_window_);

			/*
			 * If more than half of the window is in-use, increase remote window size
			 * by the amount that we have buffered.
			 */
			if ((channel->remote_window_.length() * 2) >= channel->remote_window_size_) {
				msg.append(SSH::Message::ConnectionChannelWindowAdjust);
				SSH::UInt32::encode(&msg, channel->remote_channel_);
				SSH::UInt32::encode(&msg, channel->remote_window_.length());
				pipe_->send(&msg);

				channel->remote_window_.clear();
			}
			break;
		default:
			DEBUG(log_) << "Unhandled message:" << std::endl << e.buffer_.hexdump();
			break;
		}

		EventCallback *rcb = callback(this, &SSHConnection::receive_complete);
		receive_action_ = pipe_->receive(rcb);
	}

	void close_complete(void)
	{
		close_action_->cancel();
		close_action_ = NULL;

		ASSERT(log_, peer_ != NULL);
		delete peer_;
		peer_ = NULL;

		delete this;
	}

	void splice_complete(Event e)
	{
		splice_action_->cancel();
		splice_action_ = NULL;

		switch (e.type_) {
		case Event::EOS:
			DEBUG(log_) << "Peer exiting normally.";
			break;
		case Event::Error:
			ERROR(log_) << "Peer exiting with error: " << e;
			break;
		default:
			ERROR(log_) << "Peer exiting with unknown event: " << e;
			break;
		}

	        ASSERT(log_, splice_ != NULL);
		delete splice_;
		splice_ = NULL;

		if (receive_action_ != NULL) {
			INFO(log_) << "Peer exiting while waiting for a packet.";

			receive_action_->cancel();
			receive_action_ = NULL;
		}

		ASSERT(log_, pipe_ != NULL);
		delete pipe_;
		pipe_ = NULL;

		ASSERT(log_, close_action_ == NULL);
		SimpleCallback *cb = callback(this, &SSHConnection::close_complete);
		close_action_ = peer_->close(cb);
	}

	uint32_t channel_setup(const uint32_t& remote_channel, const uint32_t& window_size, const uint32_t& packet_size)
	{
		std::map<uint32_t, SSHChannel *>::const_iterator it;
		uint32_t local_channel;

		uint32_t next = channel_next_;
		for (;;) {
			it = channel_map_.find(next);
			if (it == channel_map_.end())
				break;
			next++;
		}
		channel_next_ = next + 1;

		local_channel = next;
		channel_map_[local_channel] = new SSHChannel(local_channel, remote_channel, 65536, 65536, window_size, packet_size);
		return (local_channel);
	}
};

class SSHServer : public SimpleServer<TCPServer> {
public:
	SSHServer(SocketAddressFamily family, const std::string& interface)
	: SimpleServer<TCPServer>("/ssh/server", SocketImplOS, family, interface)
	{ }

	~SSHServer()
	{ }

	void client_connected(Socket *client)
	{
		new SSHConnection(client);
	}
};

int
main(void)
{
	new SSHServer(SocketAddressFamilyIP, "[::]:2299");
	event_main();
}
