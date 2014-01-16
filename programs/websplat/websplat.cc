/*
 * Copyright (c) 2011-2012 Juli Mallett. All rights reserved.
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

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

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

		std::string canonical_uri("/");
		Buffer::join(path_stack, "/").extract(canonical_uri);
		std::string path = config_.root_ + canonical_uri;

		handle_file(canonical_uri, path);
	}

	void handle_file(const std::string& uri, const std::string& path)
	{
		DEBUG(log_) << "Attempting to send file: " << path;

		int fd;
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1) {
			/* XXX Check errno.  */
			pipe_->send_response(HTTPProtocol::NotFound, std::string("open: error [") + strerror(errno) + "] for path: " + path);
			return;
		}

		handle_file(uri, path, fd);
	}

	void handle_file(const std::string& uri, const std::string& path, int fd)
	{
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
			if (uri == "/")
				handle_directory(uri, path);
			else
				handle_directory(uri + "/", path);
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

		Buffer headers;
		headers << "Content-length: " << data.length() << "\r\n";

		pipe_->send_response(HTTPProtocol::OK, &data, &headers);
	}

	void handle_directory(const std::string& uri_base, const std::string& directory)
	{
		static const char *index_files[] = {
			"index.html",
			NULL,
		};
		const char **ifp;

		for (ifp = index_files; *ifp != NULL; ifp++) {
			std::string index_file(directory + "/" + *ifp);

			int fd;
			fd = open(index_file.c_str(), O_RDONLY);
			if (fd == -1)
				continue;
			handle_file(uri_base + *ifp, index_file, fd);
			return;
		}

		DIR *dir;
		dir = opendir(directory.c_str());
		if (dir == NULL) {
			pipe_->send_response(HTTPProtocol::NotFound, "Could not open directory.");
			return;
		}

		Buffer data;
		data << "<html><head>";
		data << "<title>Index of " << uri_base << "</title>";
		data << "<style type=\"text/css\">";
		data << "body { font-family: sans-serif; };";
		data << "li { font-family: serif; };";
		data << "</style>";
		data << "</head>";

		data << "<body>";
		data << "<h1>" << uri_base << "</h1>";
		data << "<ul>";

		struct dirent *de;
		while ((de = readdir(dir)) != NULL)
			data << "<li><a href=\"" << uri_base << de->d_name << "\">" << de->d_name << "</a></li>";

		data << "</ul>";
		data << "</body>";
		data << "</html>";

		closedir(dir);

		Buffer headers;
		headers << "Content-length: " << data.length() << "\r\n";
		headers << "Content-type: text/html\r\n";

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

	new HTTPServer<TCPServer, WebsplatClient, WebsplatConfig>(WebsplatConfig(root), SocketImplOS, SocketAddressFamilyIP, interface);

	event_main();
}

static void
usage(void)
{
	INFO("/websplat/usage") << "websplat [-q | -v] -i interface -r root";
	exit(1);
}
