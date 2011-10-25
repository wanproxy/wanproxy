#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <io/net/tcp_server.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include <io/socket/simple_server.h>

#include <vis.h>

struct WebsplatConfig {
	std::string root_;

	WebsplatConfig(void)
	: root_("")
	{ }
};

struct HTTPMessage {
	Buffer start_line_;
	std::map<std::string, std::vector<Buffer> > headers_;
};

typedef class TypedPairCallback<Event, HTTPMessage> HTTPMessageEventCallback;

class HTTPServerPipe : public PipeProducer {
	enum State {
		GetStart,
		GetHeaders,

		GotMessage,
		Error
	};
public:
	enum Status {
		OK,
		BadRequest,
		NotFound,
		NotImplemented,
		VersionNotSupported,
	};

private:
	State state_;
	Buffer buffer_;

	HTTPMessage message_;
	std::string last_header_;

	Action *action_;
	HTTPMessageEventCallback *callback_;
public:
	HTTPServerPipe(const LogHandle& log)
	: PipeProducer(log),
	  state_(GetStart),
	  buffer_(),
	  message_(),
	  last_header_(),
	  action_(NULL),
	  callback_(NULL)
	{ }

	~HTTPServerPipe()
	{ }

	Action *message(HTTPMessageEventCallback *cb)
	{
		ASSERT(action_ == NULL);
		ASSERT(callback_ == NULL);

		if (state_ == GotMessage || state_ == Error)
			return (schedule_callback(cb));

		callback_ = cb;
		return (cancellation(this, &HTTPServerPipe::cancel));
	}

	void send_response(Status status, Buffer *body, Buffer *headers)
	{
		if (state_ == GotMessage || state_ == Error) {
			ASSERT(buffer_.empty());
		} else {
			if (!buffer_.empty()) {
				buffer_.clear();
			} else {
				ASSERT(status == BadRequest);
				DEBUG(log_) << "Premature end-of-stream.";
			}
			state_ = Error; /* Process no more input.  */

			/*
			 * For state change to Error, we must schedule a
			 * callback.
			 */
			ASSERT(action_ == NULL);
			if (callback_ != NULL) {
				action_ = schedule_callback(callback_);
				callback_ = NULL;
			}
		}

		/*
		 * Format status line.
		 */
		Buffer response("HTTP/1.1 ");
		switch (status) {
		case OK:
			response.append("200 OK");
			break;
		case BadRequest:
			response.append("400 Bad Request");
			break;
		case NotFound:
			response.append("404 Not Found");
			break;
		case NotImplemented:
			response.append("501 Not Implemented");
			break;
		case VersionNotSupported:
			response.append("505 Version Not Supported");
			break;
		default:
			NOTREACHED();
		}
		response.append("\r\n");

		/*
		 * Fill response headers.
		 */
		response.append("Server: " + (std::string)log_ + "\r\n");
		response.append("Transfer-encoding: identity\r\n");
		response.append("Connection: close\r\n");
		if (headers != NULL) {
			response.append(headers);
			headers->clear();
		}
		response.append("\r\n");

		/*
		 * Append body.  No encodings supported yet.
		 */
		response.append(body);
		body->clear();

		/*
		 * Output response and EOS.
		 *
		 * XXX Really want produce_eos(&response);
		 */
		produce(&response);
		ASSERT(response.empty());

		Buffer eos;
		produce(&eos);
	}

	void send_response(Status status, const std::string& body, const std::string& content_type = "text/plain")
	{
		Buffer tmp(body);
		Buffer header("Content-type: " + content_type + "\r\n");
		send_response(status, &tmp, &header);
		ASSERT(tmp.empty());
	}

private:
	void cancel(void)
	{
		if (action_ != NULL) {
			action_->cancel();
			action_ = NULL;

			ASSERT(callback_ == NULL);
		} else {
			ASSERT(callback_ != NULL);
			delete callback_;
			callback_ = NULL;
		}
	}

	Action *schedule_callback(HTTPMessageEventCallback *cb)
	{
		switch (state_) {
		case GotMessage:
			cb->param(Event::Done, message_);
			break;
		case Error:
			cb->param(Event::Error, HTTPMessage());
			break;
		default:
			NOTREACHED();
		}

		return (cb->schedule());
	}

	void consume(Buffer *in)
	{
		if (state_ == GotMessage || state_ == Error) {
			ASSERT(buffer_.empty());
			if (in->empty()) {
				DEBUG(log_) << "Got end-of-stream.";
				return;
			}

			/*
			 * XXX
			 * Really want a way to shut down input.
			 */
			if (state_ == GotMessage) {
				ERROR(log_) << "Client sent unexpected additional data after request.";
				state_ = Error;

				/*
				 * No need to schedule a callback for this state
				 * change, the change to GotMessage already did.
				 */
			} else {
				ERROR(log_) << "Client continuing to send gibberish.";
			}
			in->clear();
			return;
		}

		if (in->empty()) {
			send_response(BadRequest, "Premature end of request.");
			return;
		}

		buffer_.append(in);
		in->clear();

		for (;;) {
			ASSERT(!buffer_.empty());

			Buffer line;
			unsigned pos;
			/*
			 * XXX
			 * Need a find_any variant that indicates which was
			 * found.
			 */
			if (buffer_.find('\r', &pos)) {
				/* CRLF line endings.  */
				ASSERT(buffer_.length() > pos);
				if (buffer_.length() == pos + 1) {
					DEBUG(log_) << "Carriage return at end of buffer, waiting for line feed.";
					return;
				}
				if (pos != 0)
					buffer_.moveout(&line, pos);
				buffer_.skip(2);
			} else {
				if (!buffer_.find('\n', &pos)) {
					DEBUG(log_) << "Waiting for remainder of line.";
					return;
				}
				/* Unix line endings.  */
				buffer_.moveout(&line, pos);
				buffer_.skip(1);
			}

			if (state_ == GetStart) {
				if (line.empty()) {
					ERROR(log_) << "Premature end of headers.";
					return;
				}
				message_.start_line_ = line;
				state_ = GetHeaders;

				if (buffer_.empty())
					break;
				continue;
			}

			ASSERT(state_ == GetHeaders);

			/*
			 * Process end of headers!
			 */
			if (line.empty()) {
				if (!buffer_.empty()) {
					ERROR(log_) << "Client sent garbage after message.";
					send_response(BadRequest, "Garbage after message.");
					return;
				}

				/*
				 * We have received the full message.  Process any
				 * pending callback.
				 */
				state_ = GotMessage;

				ASSERT(action_ == NULL);
				if (callback_ != NULL) {
					action_ = schedule_callback(callback_);
					callback_ = NULL;
				}
				return;
			}

			/*
			 * Process header.
			 */
			if (line.peek() == ' ') { /* XXX isspace? */
				/*
				 * Fold headers per RFC822.
				 * 
				 * XXX Always forget how to handle leading whitespace.
				 */
				if (last_header_ == "") {
					send_response(BadRequest, "Folded header sent before any others.");
					return;
				}

				message_.headers_[last_header_].back().append(line);
				if (buffer_.empty())
					break;
				continue;
			}

			if (!line.find(':', &pos)) {
				send_response(BadRequest, "Empty header name.");
				return;
			}

			Buffer key;
			line.moveout(&key, pos);
			line.skip(1);

			Buffer value;
			while (!line.empty() && line.peek() == ' ')
				line.skip(1);
			value = line;

			std::string header;
			key.extract(header);

			message_.headers_[header].push_back(value);
			last_header_ = header;

			if (buffer_.empty())
				break;
		}
	}
};


class WebsplatClient {
	LogHandle log_;
	Socket *client_;
	WebsplatConfig config_;
	HTTPServerPipe *pipe_;
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
	Action *message_action_;
public:
	WebsplatClient(Socket *client, const WebsplatConfig& config)
	: log_("/websplat/server/client/" + client->getpeername()),
	  client_(client),
	  config_(config),
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

	void request(Event e, HTTPMessage message)
	{
		message_action_->cancel();
		message_action_ = NULL;

		if (e.type_ == Event::Error) {
			ERROR(log_) << "Error while waiting for request message: " << e;
			return;
		}
		ASSERT(e.type_ == Event::Done);

		ASSERT(!message.start_line_.empty());
		Buffer line(message.start_line_);

		std::string method;
		for (;;) {
			char ch = line.peek();
			line.skip(1);
			if (isspace(ch)) {
				for (;;) {
					if (line.empty()) {
						pipe_->send_response(HTTPServerPipe::BadRequest, "Premature end-of-line after method.");
						return;
					}
					ch = line.peek();
					if (!isspace(ch))
						break;
					line.skip(1);
				}
				break;
			}
			if (line.empty()) {
				pipe_->send_response(HTTPServerPipe::BadRequest, "End-of-line inside method.");
				return;
			}
			method += toupper(ch);
		}

		std::string uri;
		for (;;) {
			char ch = line.peek();
			line.skip(1);
			if (isspace(ch)) {
				for (;;) {
					if (line.empty()) {
						pipe_->send_response(HTTPServerPipe::BadRequest, "Premature end-of-line after URI.");
						return;
					}
					ch = line.peek();
					if (!isspace(ch))
						break;
					line.skip(1);
				}
				break;
			}
			if (line.empty()) {
				pipe_->send_response(HTTPServerPipe::BadRequest, "End-of-line inside URI.");
				return;
			}
			uri += ch;
		}

		if (!line.equal("HTTP/1.1") && !line.equal("HTTP/1.0")) {
			pipe_->send_response(HTTPServerPipe::VersionNotSupported, "Version not supported.");
			return;
		}

		char uri_decoded[uri.length()];
		int rv = strunvisx(uri_decoded, uri.c_str(), VIS_HTTPSTYLE);
		if (rv == -1) {
			pipe_->send_response(HTTPServerPipe::BadRequest, "Malformed URI encoding.");
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

	void handle_request(const std::string& method, const std::string& uri, HTTPMessage)
	{
		if (method != "GET") {
			pipe_->send_response(HTTPServerPipe::NotImplemented, "Unsupported method.");
			return;
		}

		pipe_->send_response(HTTPServerPipe::OK, "<html><head><title>Websplat - " + uri + "</title></head><body><h1>Welcome to Websplat!</h1><p>Have a nice " + method + "</p></body></html>", "text/html");
	}
};

template<typename T>
class WebsplatServer : public SimpleServer<T> {
	WebsplatConfig config_;
public:
	WebsplatServer(SocketAddressFamily family, const std::string& interface,
		       const WebsplatConfig& config)
	: SimpleServer<T>("/websplat/server", family, interface),
	  config_(config)
	{ }

	~WebsplatServer()
	{ }

	void client_connected(Socket *client)
	{
		new WebsplatClient(client, config_);
	}
};

static void usage(void);

int
main(int argc, char *argv[])
{
	std::string interface("");
	WebsplatConfig config;
	bool quiet, verbose;
	int ch;

	quiet = false;
	verbose = false;

	INFO("/websplat") << "Websplat";
	INFO("/websplat") << "Copyright (c) 2008-2011 WANProxy.org.";
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
			config.root_ = optarg;
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

	if (config.root_ == "")
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

	new WebsplatServer<TCPServer>(SocketAddressFamilyIP, interface, config);

	event_main();
}

static void
usage(void)
{
	INFO("/websplat/usage") << "wanproxy [-q | -v] -i interface -r root";
	exit(1);
}
