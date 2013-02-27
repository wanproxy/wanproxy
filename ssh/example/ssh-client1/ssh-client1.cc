/*
 * Copyright (c) 2012 Juli Mallett. All rights reserved.
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

#include <io/net/tcp_client.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_protocol.h>
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
	  session_(SSH::ClientRole),
	  pipe_(NULL),
	  splice_(NULL),
	  splice_action_(NULL),
	  close_action_(NULL)
	{
		session_.algorithm_negotiation_ = new SSH::AlgorithmNegotiation(&session_);
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

		/*
		 * SSH Echo!
		 */
		pipe_->send(&e.buffer_);

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

class SSHClient {
	LogHandle log_;
	Action *connect_action_;
public:
	SSHClient(SocketAddressFamily family, const std::string& interface)
	: log_("/ssh/client"),
	  connect_action_(NULL)
	{
		SocketEventCallback *cb = callback(this, &SSHClient::connect_complete);
		connect_action_ = TCPClient::connect(family, interface, cb);
	}

	~SSHClient()
	{ }

	void connect_complete(Event e, Socket *client)
	{
		connect_action_->cancel();
		connect_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Connect error: " << e;
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		if (e.type_ == Event::Done)
			new SSHConnection(client);

		delete this;
	}
};


int
main(void)
{
	new SSHClient(SocketAddressFamilyIP, "localhost:22");
	event_main();
}
