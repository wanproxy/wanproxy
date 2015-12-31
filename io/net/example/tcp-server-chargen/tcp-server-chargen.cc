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

#include <unistd.h>

#include <common/thread/mutex.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <io/net/tcp_server.h>

#include <io/socket/simple_server.h>

#define	RFC864_STRING	"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ "
#define	RFC864_LEN	(sizeof RFC864_STRING - 1)
#define	WIDTH		72

class ChargenClient {
	LogHandle log_;
	Mutex mtx_;
	Socket *client_;
	Action *action_;
	Buffer data_;
public:
	ChargenClient(Socket *client)
	: log_("/chargen/client"),
	  mtx_("ChargenClient"),
	  client_(client),
	  action_(NULL),
	  data_()
	{
		unsigned i;

		for (i = 0; i < RFC864_LEN; i++) {
			size_t resid = WIDTH;

			if (RFC864_LEN - i >= resid) {
				data_.append((const uint8_t *)&RFC864_STRING[i], resid);
			} else {
				data_.append((const uint8_t *)&RFC864_STRING[i], RFC864_LEN - i);
				resid -= RFC864_LEN - i;
				data_.append((const uint8_t *)RFC864_STRING, resid);
			}
			data_.append("\r\n");
		}

		INFO(log_) << "Client connected: " << client_->getpeername();

		ScopedLock _(&mtx_);
		EventCallback *cb = callback(&mtx_, this, &ChargenClient::shutdown_complete);
		action_ = client_->shutdown(true, false, cb);
	}

	~ChargenClient()
	{
		ASSERT_NULL(log_, client_);
		ASSERT_NULL(log_, action_);
	}

	void close_complete(void)
	{
		ASSERT_LOCK_OWNED(log_, &mtx_);
		action_->cancel();
		action_ = NULL;

		ASSERT_NON_NULL(log_, client_);
		delete client_;
		client_ = NULL;

		delete this;
	}

	void shutdown_complete(Event e)
	{
		ASSERT_LOCK_OWNED(log_, &mtx_);
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Error during shutdown: " << e;
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		schedule_write();
	}

	void write_complete(Event e)
	{
		ASSERT_LOCK_OWNED(log_, &mtx_);
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Error during write: " << e;
			schedule_close();
			return;
		default:
			HALT(log_) << "Unexpected event: " << e;
			schedule_close();
			return;
		}

		schedule_write();
	}

	void schedule_close(void)
	{
		ASSERT_LOCK_OWNED(log_, &mtx_);
		ASSERT_NULL(log_, action_);
		ASSERT_NON_NULL(log_, client_);

		SimpleCallback *cb = callback(&mtx_, this, &ChargenClient::close_complete);
		action_ = client_->close(cb);
	}

	void schedule_write(void)
	{
		ASSERT_LOCK_OWNED(log_, &mtx_);
		ASSERT_NULL(log_, action_);
		ASSERT_NON_NULL(log_, client_);

		Buffer tmp(data_);
		EventCallback *cb = callback(&mtx_, this, &ChargenClient::write_complete);
		action_ = client_->write(&tmp, cb);
	}
};

class ChargenListener : public SimpleServer<TCPServer> {
public:
	ChargenListener(void)
	: SimpleServer<TCPServer>("/chargen/listener", SocketImplOS, SocketAddressFamilyIP, "[::]:0")
	{ }

	~ChargenListener()
	{ }

	void client_connected(Socket *socket)
	{
		new ChargenClient(socket);
	}
};

int
main(void)
{
	new ChargenListener();

	event_main();
}
