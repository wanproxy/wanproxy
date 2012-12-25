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

class SSHConnection {
	LogHandle log_;
	Socket *peer_;
	SSH::Session session_;
	SSH::TransportPipe *pipe_;
	Action *receive_action_;
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
public:
	SSHConnection(Socket *peer)
	: log_("/ssh/connection"),
	  peer_(peer),
	  session_(SSH::ServerRole),
	  pipe_(NULL),
	  splice_(NULL),
	  splice_action_(NULL),
	  close_action_(NULL)
	{
		session_.algorithm_negotiation_ = new SSH::AlgorithmNegotiation(&session_);
		if (session_.role_ == SSH::ServerRole) {
			SSH::ServerHostKey *server_host_key = SSH::ServerHostKey::server(&session_, "ssh-server1.pem");
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
		Buffer msg;
		uint32_t recipient_channel, sender_channel, window_size, packet_size;
		switch (e.buffer_.peek()) {
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

			/* Set up session.  */
			msg.append(SSH::Message::ConnectionChannelOpenConfirmation);
			SSH::UInt32::encode(&msg, sender_channel);
			SSH::UInt32::encode(&msg, sender_channel); /* Use the same channel as the client.  */
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
			/* XXX Ignore rest of request for now.  */

			msg.append(SSH::Message::ConnectionChannelRequestFailure);
			SSH::UInt32::encode(&msg, recipient_channel); /* Sender and recipient channels match.  */
			pipe_->send(&msg);
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
};

class SSHServer : public SimpleServer<TCPServer> {
public:
	SSHServer(SocketAddressFamily family, const std::string& interface)
	: SimpleServer<TCPServer>("/ssh/server", family, interface)
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
