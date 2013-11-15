/*
 * Copyright (c) 2011 Juli Mallett. All rights reserved.
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

#ifndef	IO_SOCKET_SIMPLE_SERVER_H
#define	IO_SOCKET_SIMPLE_SERVER_H

#include <io/socket/socket.h>

/*
 * XXX
 * This is just one level up from using macros.  Would be nice to use abstract
 * base classes and something a bit tidier.
 */
template<typename L>
class SimpleServer {
	LogHandle log_;
	L *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
public:
	SimpleServer(LogHandle log, SocketImpl impl, SocketAddressFamily family, const std::string& interface)
	: log_(log),
	  server_(NULL),
	  accept_action_(NULL),
	  close_action_(NULL),
	  stop_action_(NULL)
	{
		server_ = L::listen(impl, family, interface);
		if (server_ == NULL)
			HALT(log_) << "Unable to create listener.";

		INFO(log_) << "Listening on: " << server_->getsockname();

		SocketEventCallback *cb = callback(this, &SimpleServer::accept_complete);
		accept_action_ = server_->accept(cb);

		SimpleCallback *scb = callback(this, &SimpleServer::stop);
		stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
	}

	virtual ~SimpleServer()
	{
		ASSERT(log_, server_ == NULL);
		ASSERT(log_, accept_action_ == NULL);
		ASSERT(log_, close_action_ == NULL);
		ASSERT(log_, stop_action_ == NULL);
	}

private:
	void accept_complete(Event e, Socket *client)
	{
		accept_action_->cancel();
		accept_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Accept error: " << e;
			break;
		default:
			ERROR(log_) << "Unexpected event: " << e;
			break;
		}

		if (e.type_ == Event::Done) {
			DEBUG(log_) << "Accepted client: " << client->getpeername();
			client_connected(client);
		}

		SocketEventCallback *cb = callback(this, &SimpleServer::accept_complete);
		accept_action_ = server_->accept(cb);
	}

	void close_complete(void)
	{
		close_action_->cancel();
		close_action_ = NULL;

		ASSERT(log_, server_ != NULL);
		delete server_;
		server_ = NULL;

		delete this;
	}

	void stop(void)
	{
		stop_action_->cancel();
		stop_action_ = NULL;

		accept_action_->cancel();
		accept_action_ = NULL;

		ASSERT(log_, close_action_ == NULL);

		SimpleCallback *cb = callback(this, &SimpleServer::close_complete);
		close_action_ = server_->close(cb);
	}

	virtual void client_connected(Socket *) = 0;
};

#endif /* !IO_SOCKET_SIMPLE_SERVER_H */
