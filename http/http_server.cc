/*
 * Copyright (c) 2011-2013 Juli Mallett. All rights reserved.
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
#include <event/event_main.h>
#include <event/event_system.h>

#include <http/http_server.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

HTTPServerHandler::HTTPServerHandler(Socket *client)
: log_("/http/server/handler/" + client->getpeername()),
  mtx_("HTTPServerHandler"),
  client_(client),
  pipe_(NULL),
  splice_(NULL),
  splice_action_(NULL),
  close_complete_(NULL, &mtx_, this, &HTTPServerHandler::close_complete),
  close_action_(NULL),
  request_action_(NULL)
{
	ScopedLock _(&mtx_);
	pipe_ = new HTTPServerPipe(log_ + "/pipe");
	HTTPRequestEventCallback *hcb = callback(&mtx_, this, &HTTPServerHandler::request);
	request_action_ = pipe_->request(hcb);

	splice_ = new Splice(log_, client_, pipe_, client_);
	EventCallback *scb = callback(&mtx_, this, &HTTPServerHandler::splice_complete);
	splice_action_ = splice_->start(scb);
}

HTTPServerHandler::~HTTPServerHandler()
{
	ASSERT_NULL(log_, pipe_);
	ASSERT_NULL(log_, splice_);
	ASSERT_NULL(log_, splice_action_);
	ASSERT_NULL(log_, close_action_);
	ASSERT_NULL(log_, request_action_);
}

void
HTTPServerHandler::close_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	close_action_->cancel();
	close_action_ = NULL;

	ASSERT_NON_NULL(log_, client_);
	delete client_;
	client_ = NULL;

	EventSystem::instance()->destroy(&mtx_, this);
}

void
HTTPServerHandler::request(Event e, HTTPProtocol::Request req)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	request_action_->cancel();
	request_action_ = NULL;

	if (e.type_ == Event::Error) {
		ERROR(log_) << "Error while waiting for request: " << e;
		return;
	}
	ASSERT(log_, e.type_ == Event::Done);

	ASSERT(log_, !req.start_line_.empty());
	std::vector<Buffer> words = req.start_line_.split(' ', false);
	ASSERT(log_, !words.empty());

	std::string method;
	words[0].toupper().extract(method);
	ASSERT(log_, !method.empty());

	Buffer uri_encoded(words[1]);
	ASSERT(log_, !uri_encoded.empty());
	Buffer uri_decoded;
	if (!HTTPProtocol::DecodeURI(&uri_encoded, &uri_decoded)) {
		pipe_->send_response(HTTPProtocol::BadRequest, "Malformed URI encoding.");
		return;
	}
	ASSERT(log_, !uri_decoded.empty());

	if (words.size() == 3) {
		std::string version;
		words[2].extract(version);
		ASSERT(log_, !version.empty());

		if (version != "HTTP/1.1" && version != "HTTP/1.0") {
			pipe_->send_response(HTTPProtocol::VersionNotSupported, "Version not supported.");
			return;
		}
	}

	std::string uri;
	uri_decoded.extract(uri);
	ASSERT(log_, !uri.empty());

	handle_request(method, uri, req);
}

void
HTTPServerHandler::splice_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	splice_action_->cancel();
	splice_action_ = NULL;

	switch (e.type_) {
	case Event::EOS:
		DEBUG(log_) << "Client exiting normally.";
		break;
	case Event::Error:
		ERROR(log_) << "Client exiting with error: " << e;
		break;
	default:
		ERROR(log_) << "Client exiting with unknown event: " << e;
		break;
	}

	ASSERT_NON_NULL(log_, splice_);
	delete splice_;
	splice_ = NULL;

	if (request_action_ != NULL) {
		ERROR(log_) << "Splice exited before request was received.";
		request_action_->cancel();
		request_action_ = NULL;
	}

	ASSERT_NON_NULL(log_, pipe_);
	delete pipe_;
	pipe_ = NULL;

	ASSERT_NULL(log_, close_action_);
	close_action_ = client_->close(&close_complete_);
}
