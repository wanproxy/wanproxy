#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <http/http_server_pipe.h>

#include <io/net/tcp_server.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include <io/socket/simple_server.h>

#include <vis.h>

class WebsplatClient {
	LogHandle log_;
	Socket *client_;
	HTTPServerPipe *pipe_;
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
	Action *message_action_;
public:
	WebsplatClient(Socket *client)
	: log_("/websplat/server/client/" + client->getpeername()),
	  client_(client),
	  pipe_(NULL),
	  splice_(NULL),
	  splice_action_(NULL),
	  close_action_(NULL),
	  message_action_(NULL)
	{
		pipe_ = new HTTPServerPipe(log_ + "/pipe");
		HTTPMessageEventCallback *hcb = callback(this, &WebsplatClient::request);
		message_action_ = pipe_->message(hcb);

		splice_ = new Splice(log_, client_, pipe_, client_);
		EventCallback *scb = callback(this, &WebsplatClient::splice_complete);
		splice_action_ = splice_->start(scb);
	}

	~WebsplatClient()
	{
		ASSERT(pipe_ == NULL);
		ASSERT(splice_ == NULL);
		ASSERT(splice_action_ == NULL);
		ASSERT(close_action_ == NULL);
		ASSERT(message_action_ == NULL);
	}

private:
	void close_complete(void)
	{
		close_action_->cancel();
		close_action_ = NULL;

		ASSERT(client_ != NULL);
		delete client_;
		client_ = NULL;

		delete this;
	}

	void request(Event e, HTTPProtocol::Message message)
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

		if (words.empty()) {
			pipe_->send_response(HTTPProtocol::BadRequest, "Empty request.");
			return;
		}

		if (words.size() != 3) {
			pipe_->send_response(HTTPProtocol::BadRequest, "Wrong number of parameters in request.");
			return;
		}

		std::string method;
		words[0].extract(method);
		ASSERT(!method.empty());
		std::transform(method.begin(), method.end(), method.begin(), ::toupper);

		std::string uri;
		words[1].extract(uri);
		ASSERT(!uri.empty());

		std::string version;
		words[2].extract(version);
		ASSERT(!version.empty());

		if (version != "HTTP/1.1" && version != "HTTP/1.0") {
			pipe_->send_response(HTTPProtocol::VersionNotSupported, "Version not supported.");
			return;
		}

		char uri_decoded[uri.length()];
		int rv = strunvisx(uri_decoded, uri.c_str(), VIS_HTTPSTYLE);
		if (rv == -1) {
			pipe_->send_response(HTTPProtocol::BadRequest, "Malformed URI encoding.");
			return;
		}
		ASSERT(rv >= 0);

		handle_request(method, std::string(uri_decoded, rv), message);
	}

	void splice_complete(Event e)
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
		SimpleCallback *cb = callback(this, &WebsplatClient::close_complete);
		close_action_ = client_->close(cb);
	}

	void handle_request(const std::string& method, const std::string& uri, HTTPProtocol::Message)
	{
		if (method != "GET") {
			pipe_->send_response(HTTPProtocol::NotImplemented, "Unsupported method.");
			return;
		}

		if (uri.empty() || uri[0] != '/') {
			pipe_->send_response(HTTPProtocol::BadRequest, "Invalid URI.");
			return;
		}

		std::vector<Buffer> path_components = Buffer(uri).split('/', false);
		std::vector<Buffer>::const_iterator it;
		std::vector<Buffer> path_stack;
		for (it = path_components.begin(); it != path_components.end(); ++it) {
			Buffer component = *it;

			if (component.equal("."))
				continue;

			if (component.equal("..")) {
				if (path_stack.empty()) {
					pipe_->send_response(HTTPProtocol::BadRequest, "Gratuitously-invalid URI.");
					return;
				}
				path_stack.pop_back();
			}

			path_stack.push_back(component);
		}
		path_components.clear();

		pipe_->send_response(HTTPProtocol::OK, "<html><head><title>Websplat - " + uri + "</title></head><body><h1>Welcome to Websplat!</h1><p>Have a nice " + method + "</p></body></html>", "text/html");
	}
};

template<typename T>
class WebsplatServer : public SimpleServer<T> {
public:
	WebsplatServer(SocketAddressFamily family, const std::string& interface)
	: SimpleServer<T>("/websplat/server", family, interface)
	{ }

	~WebsplatServer()
	{ }

	void client_connected(Socket *client)
	{
		new WebsplatClient(client);
	}
};

static void usage(void);

int
main(int argc, char *argv[])
{
	std::string interface("");
	bool quiet, verbose;
	int ch;

	quiet = false;
	verbose = false;

	INFO("/websplat") << "Websplat";
	INFO("/websplat") << "Copyright (c) 2008-2011 WANProxy.org.";
	INFO("/websplat") << "All rights reserved.";

	while ((ch = getopt(argc, argv, "i:qv")) != -1) {
		switch (ch) {
		case 'i':
			interface = optarg;
			break;
		case 'q':
			quiet = true;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
		}
	}

	if (interface == "")
		usage();

	if (quiet && verbose)
		usage();

	if (verbose) {
		Log::mask(".?", Log::Debug);
	} else if (quiet) {
		Log::mask(".?", Log::Error);
	} else {
		Log::mask(".?", Log::Info);
	}

	new WebsplatServer<TCPServer>(SocketAddressFamilyIP, interface);

	event_main();
}

static void
usage(void)
{
	INFO("/websplat/usage") << "wanproxy [-q | -v] -i interface";
	exit(1);
}
