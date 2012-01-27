#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

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

struct WebsplatConfig {
	std::string root_;

	WebsplatConfig(const std::string& root)
	: root_(root)
	{ }
};

class WebsplatClient : public HTTPServerHandler {
	WebsplatConfig config_;
public:
	WebsplatClient(const WebsplatConfig& config, Socket *client)
	: HTTPServerHandler(client),
	  config_(config)
	{ }

	~WebsplatClient()
	{ }

private:
	void handle_request(const std::string& method, const std::string& uri, HTTPProtocol::Request)
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

		std::string path = config_.root_ + "/";
		Buffer::join(path_stack, "/").extract(path);

		handle_file(path);
	}

	void handle_file(const std::string& path)
	{
		DEBUG(log_) << "Attempting to send file: " << path;

		int fd;
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1) {
			/* XXX Check errno.  */
			pipe_->send_response(HTTPProtocol::NotFound, std::string("open: error [") + strerror(errno) + "] for path: " + path);
			return;
		}

		struct stat sb;
		int rv = fstat(fd, &sb);
		if (rv != 0) {
			close(fd);
			pipe_->send_response(HTTPProtocol::NotFound, std::string("fstat: error [") + strerror(errno) + "] for path: " + path);
			return;
		}

		switch (sb.st_mode & S_IFMT) {
		case S_IFREG:
			break;
		case S_IFDIR:
			close(fd);
			handle_file(path + "/index.html");
			return;
		default:
			close(fd);
			pipe_->send_response(HTTPProtocol::NotFound, "No file was found at that path, wink-wink, nudge-nudge.");
			return;
		}

		Buffer data;
		for (;;) {
			uint8_t buf[65536];
			ssize_t amt = read(fd, buf, sizeof buf);
			if (amt == 0)
				break;
			if (amt == -1) {
				close(fd);
				pipe_->send_response(HTTPProtocol::NotFound, "Could not read file.");
				return;
			}
			data.append(buf, (size_t)amt);
		}
		close(fd);

		std::ostringstream os;
		os << "Content-length: " << data.length() << "\r\n";
		Buffer headers(os.str());

		pipe_->send_response(HTTPProtocol::OK, &data, &headers);
	}
};

static void usage(void);

int
main(int argc, char *argv[])
{
	std::string interface("");
	std::string root("");
	bool quiet, verbose;
	int ch;

	quiet = false;
	verbose = false;

	INFO("/websplat") << "Websplat";
	INFO("/websplat") << "Copyright (c) 2008-2012 WANProxy.org.";
	INFO("/websplat") << "All rights reserved.";

	while ((ch = getopt(argc, argv, "i:qr:v")) != -1) {
		switch (ch) {
		case 'i':
			interface = optarg;
			break;
		case 'q':
			quiet = true;
			break;
		case 'r':
			root = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
		}
	}

	if (interface == "" || root == "")
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

	new HTTPServer<TCPServer, WebsplatClient, WebsplatConfig>(WebsplatConfig(root), SocketAddressFamilyIP, interface);

	event_main();
}

static void
usage(void)
{
	INFO("/websplat/usage") << "websplat [-q | -v] -i interface -r root";
	exit(1);
}
