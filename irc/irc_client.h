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

#ifndef	IRC_IRC_CLIENT_H
#define	IRC_IRC_CLIENT_H

#include <irc/irc_command.h>

class Socket;

class IRCClient {
protected:
	LogHandle log_;
	Mutex mtx_;

private:
	/*
	 * XXX
	 * We should take socket parameters and a server name, not
	 * just a server hostname/IP.
	 *
	 * XXX
	 * Other parameters associated with a connectioon should be here.
	 */
	std::string server_;
	std::string nick_;
	std::string name_;

	Socket *socket_;
	Action *close_action_;
	Action *connect_action_;

	Action *read_action_;
	Buffer read_buffer_;

	Action *write_action_;
	Buffer write_buffer_;
protected:
	IRCClient(const std::string&, const std::string&, const std::string& = "");

public:
	/* XXX This should be private, but compilers complain.  */
	virtual ~IRCClient();

private:
	void schedule_close(void);
	void close_complete(void);

	void schedule_connect(void);
	void connect_complete(Event, Socket *);

	void schedule_read(void);
	void read_complete(Event);

	void schedule_write(void);
	void write_complete(Event);

protected:
	void send_command(const IRC::Command&);

	virtual bool handle_close(void)
	{
		/*
		 * By default, do nothing at close but reconnect.
		 */
		return (true);
	}

	virtual void handle_command(const IRC::Command&) = 0;
};

#endif /* !IRC_IRC_CLIENT_H */
