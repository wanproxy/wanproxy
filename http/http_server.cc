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
  client_(client),
  pipe_(NULL),
  splice_(NULL),
  splice_action_(NULL),
  close_action_(NULL),
  message_action_(NULL)
{
	pipe_ = new HTTPServerPipe(log_ + "/pipe");
	HTTPMessageEventCallback *hcb = callback(this, &HTTPServerHandler::request);
	message_action_ = pipe_->message(hcb);

	splice_ = new Splice(log_, client_, pipe_, client_);
	EventCallback *scb = callback(this, &HTTPServerHandler::splice_complete);
	splice_action_ = splice_->start(scb);
}

HTTPServerHandler::~HTTPServerHandler()
{
	ASSERT(pipe_ == NULL);
	ASSERT(splice_ == NULL);
	ASSERT(splice_action_ == NULL);
	ASSERT(close_action_ == NULL);
	ASSERT(message_action_ == NULL);
}

void
HTTPServerHandler::close_complete(void)
{
	close_action_->cancel();
	close_action_ = NULL;

	ASSERT(client_ != NULL);
	delete client_;
	client_ = NULL;

	delete this;
}

void
HTTPServerHandler::request(Event e, HTTPProtocol::Message message)
{
	message_action_->cancel();
	message_action_ = NULL;

	if (e.type_ == Event::Error) {
		ERROR(log_) << "Error while waiting for request message: " << e;
		return;
	}
	ASSERT(e.type_ == Event::Done);

	ASSERT(!message.start_line_.empty());
	std::vector<Buffer> words = message.start_line_.split(' ', false);
	ASSERT(!words.empty());

	std::string method;
	words[0].extract(method);
	ASSERT(!method.empty());
	std::transform(method.begin(), method.end(), method.begin(), ::toupper);

	Buffer uri_encoded(words[1]);
	ASSERT(!uri_encoded.empty());
	Buffer uri_decoded;
	if (!HTTPProtocol::DecodeURI(&uri_encoded, &uri_decoded)) {
		pipe_->send_response(HTTPProtocol::BadRequest, "Malformed URI encoding.");
		return;
	}
	ASSERT(!uri_decoded.empty());

	if (words.size() == 3) {
		std::string version;
		words[2].extract(version);
		ASSERT(!version.empty());

		if (version != "HTTP/1.1" && version != "HTTP/1.0") {
			pipe_->send_response(HTTPProtocol::VersionNotSupported, "Version not supported.");
			return;
		}
	}

	std::string uri;
	uri_decoded.extract(uri);
	ASSERT(!uri.empty());

	handle_request(method, uri, message);
}

void
HTTPServerHandler::splice_complete(Event e)
{
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

	ASSERT(splice_ != NULL);
	delete splice_;
	splice_ = NULL;

	if (message_action_ != NULL) {
		ERROR(log_) << "Splice exited before message was received.";
		message_action_->cancel();
		message_action_ = NULL;
	}

	ASSERT(pipe_ != NULL);
	delete pipe_;
	pipe_ = NULL;

	ASSERT(close_action_ == NULL);
	SimpleCallback *cb = callback(this, &HTTPServerHandler::close_complete);
	close_action_ = client_->close(cb);
}
