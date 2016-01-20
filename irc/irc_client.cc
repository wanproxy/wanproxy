/*
 * Copyright (c) 2012-2016 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_callback.h>
#include <event/event_system.h>

/*
 * XXX
 * The line extractor is useful and should move to BufferUtil, and not be tied
 * to the HTTP protocol.  We don't depend on HTTP.  Or RFC822.  Or any other
 * line-oriented protocol.  We just happen to share line behaviours.
 */
#include <http/http_protocol.h>

#include <irc/irc_client.h>

#include <io/net/tcp_client.h>

IRCClient::IRCClient(const std::string& server, const std::string& nick, const std::string& name)
: log_("/irc/client"),
  mtx_("IRCClient"),
  server_(server),
  nick_(nick),
  name_(name),
  socket_(NULL),
  close_action_(NULL),
  connect_action_(NULL),
  read_action_(NULL),
  read_buffer_(),
  write_action_(NULL),
  write_buffer_()
{
	ScopedLock _(&mtx_);
	schedule_connect();
}

IRCClient::~IRCClient()
{
	ASSERT_NULL(log_, socket_);
	ASSERT_NULL(log_, close_action_);
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, read_action_);
	ASSERT_ZERO(log_, read_buffer_.length());
	ASSERT_NULL(log_, write_action_);
	ASSERT_ZERO(log_, write_buffer_.length());
}

void
IRCClient::schedule_close(void)
{
	ASSERT_NULL(log_, close_action_);
	ASSERT_NON_NULL(log_, socket_);

	ASSERT_NULL(log_, connect_action_);

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}
	if (!read_buffer_.empty())
		read_buffer_.clear();

	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}
	if (!write_buffer_.empty())
		write_buffer_.clear();

	INFO(log_) << "Closing connection.";

	SimpleCallback *cb = callback(&mtx_, this, &IRCClient::close_complete);
	close_action_ = socket_->close(cb);
}

void
IRCClient::close_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	close_action_->cancel();
	close_action_ = NULL;

	delete socket_;
	socket_ = NULL;

	INFO(log_) << "Connection closed.";

	/*
	 * Notify our consumer of close, and find out whether we should
	 * try to reconnect.
	 */
	if (!handle_close()) {
		EventSystem::instance()->destroy(&mtx_, this);
		return;
	}

	schedule_connect();
}

void
IRCClient::schedule_connect(void)
{
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, socket_);

	ASSERT_ZERO(log_, read_buffer_.length());
	ASSERT_ZERO(log_, write_buffer_.length());

	INFO(log_) << "Connecting to server.";

	SocketEventCallback *cb = callback(&mtx_, this, &IRCClient::connect_complete);
	connect_action_ = TCPClient::connect(SocketImplOS, SocketAddressFamilyIP, "[" + server_ + "]:6667", cb);
}

void
IRCClient::connect_complete(Event e, Socket *socket)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	connect_action_->cancel();
	connect_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error:
		ASSERT_NULL(log_, socket);
		INFO(log_) << "Connect failed: " << e;
		schedule_connect();
		return;
	default:
		ASSERT_NULL(log_, socket);
		ERROR(log_) << "Unexpected event: " << e;
		schedule_connect();
		return;
	}

	socket_ = socket;
	ASSERT_NON_NULL(log_, socket_);

	IRC::Command user("USER");
	user.param(nick_);		/* XXX Specify username? */
	user.param("localhost");	/* XXX Specify hostname? */
	user.param(server_);
	user.param(name_);
	send_command(user);

	IRC::Command nick("NICK");
	nick.param(nick_);
	send_command(nick);

	schedule_read();
}

void
IRCClient::schedule_read(void)
{
	ASSERT_NULL(log_, read_action_);
	EventCallback *cb = callback(&mtx_, this, &IRCClient::read_complete);
	read_action_ = socket_->read(0, cb);
}

void
IRCClient::read_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::EOS:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	e.buffer_.moveout(&read_buffer_);

	if (read_buffer_.empty()) {
		ASSERT(log_, e.type_ == Event::EOS);
		schedule_close();
		return;
	}

	while (!read_buffer_.empty()) {
		Buffer line;
		switch (HTTPProtocol::ExtractLine(&line, &read_buffer_)) {
		case HTTPProtocol::ParseSuccess:
			break;
		case HTTPProtocol::ParseFailure:
			ERROR(log_) << "Invalid line from server.";
			schedule_close();
			return;
		case HTTPProtocol::ParseIncomplete:
			/* Wait for more.  */
			schedule_read();
			return;
		}

		IRC::Command c;
		if (!c.parse(&line)) {
			ERROR(log_) << "Invalid command from server.";
			schedule_close();
			return;
		}

		std::string command;
		c.command_.extract(command);

		/*
		 * We handle PING/PONG here directly.
		 */
		if (command == "PING") {
			/* Rewrite command to retain PING/PONG parameters.  */
			c.source_ = "";
			c.command_ = "PONG";
			send_command(c);
			continue;
		}

		handle_command(c);
	}

	schedule_read();
}

void
IRCClient::schedule_write(void)
{
	ASSERT_NULL(log_, write_action_);

	ASSERT_NON_ZERO(log_, write_buffer_.length());

	EventCallback *cb = callback(&mtx_, this, &IRCClient::write_complete);
	write_action_ = socket_->write(&write_buffer_, cb);
}

void
IRCClient::write_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	if (!write_buffer_.empty())
		schedule_write();
}

void
IRCClient::send_command(const IRC::Command& cmd)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	cmd.serialize(&write_buffer_);
	write_buffer_.append("\r\n");

	if (write_action_ == NULL)
		schedule_write();
}
