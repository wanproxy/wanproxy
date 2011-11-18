#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <http/http_server.h>

#include <io/net/tcp_server.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include <io/socket/simple_server.h>

class WebsplatClient : public HTTPServerHandler {
public:
	WebsplatClient(Socket *client)
	: HTTPServerHandler(client)
	{ }

	~WebsplatClient()
	{ }

private:
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

	new HTTPServer<TCPServer, WebsplatClient>(SocketAddressFamilyIP, interface);

	event_main();
}

static void
usage(void)
{
	INFO("/websplat/usage") << "wanproxy [-q | -v] -i interface";
	exit(1);
}
