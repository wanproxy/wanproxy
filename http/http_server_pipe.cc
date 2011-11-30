#include <common/buffer.h>

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

HTTPServerPipe::HTTPServerPipe(const LogHandle& log)
: PipeProducer(log),
  state_(GetStart),
  buffer_(),
  request_(),
  last_header_(),
  action_(NULL),
  callback_(NULL)
{ }

HTTPServerPipe::~HTTPServerPipe()
{
	ASSERT(action_ == NULL);
	ASSERT(callback_ == NULL);
}

Action *
HTTPServerPipe::request(HTTPRequestEventCallback *cb)
{
	ASSERT(action_ == NULL);
	ASSERT(callback_ == NULL);

	if (state_ == GotRequest || state_ == Error)
		return (schedule_callback(cb));

	callback_ = cb;
	return (cancellation(this, &HTTPServerPipe::cancel));
}

void
HTTPServerPipe::send_response(HTTPProtocol::Status status, Buffer *body, Buffer *headers)
{
	if (state_ == GotRequest || state_ == Error) {
		ASSERT(buffer_.empty());
	} else {
		if (!buffer_.empty()) {
			buffer_.clear();
		} else {
			ASSERT(status == HTTPProtocol::BadRequest);
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
	case HTTPProtocol::OK:
		response.append("200 OK");
		break;
	case HTTPProtocol::BadRequest:
		response.append("400 Bad Request");
		break;
	case HTTPProtocol::NotFound:
		response.append("404 Not Found");
		break;
	case HTTPProtocol::NotImplemented:
		response.append("501 Not Implemented");
		break;
	case HTTPProtocol::VersionNotSupported:
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

void
HTTPServerPipe::send_response(HTTPProtocol::Status status, const std::string& body, const std::string& content_type)
{
	Buffer tmp(body);
	Buffer header("Content-type: " + content_type + "\r\n");
	send_response(status, &tmp, &header);
	ASSERT(tmp.empty());
}

void
HTTPServerPipe::cancel(void)
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

Action *
HTTPServerPipe::schedule_callback(HTTPRequestEventCallback *cb)
{
	switch (state_) {
	case GotRequest:
		cb->param(Event::Done, request_);
		break;
	case Error:
		cb->param(Event::Error, HTTPProtocol::Request());
		break;
	default:
		NOTREACHED();
	}

	return (cb->schedule());
}

void
HTTPServerPipe::consume(Buffer *in)
{
	if (state_ == GotRequest || state_ == Error) {
		ASSERT(buffer_.empty());
		if (in->empty()) {
			DEBUG(log_) << "Got end-of-stream.";
			return;
		}

		/*
		 * XXX
		 * Really want a way to shut down input.
		 */
		if (state_ == GotRequest) {
			ERROR(log_) << "Client sent unexpected additional data after request.";
			state_ = Error;

			/*
			 * No need to schedule a callback for this state
			 * change, the change to GotRequest already did.
			 */
		} else {
			ERROR(log_) << "Client continuing to send gibberish.";
		}
		in->clear();
		return;
	}

	if (in->empty()) {
		send_response(HTTPProtocol::BadRequest, "Premature end of request.");
		return;
	}

	buffer_.append(in);
	in->clear();

	for (;;) {
		ASSERT(!buffer_.empty());

		Buffer line;
		switch (HTTPProtocol::ExtractLine(&line, &buffer_)) {
		case HTTPProtocol::ParseSuccess:
			break;
		case HTTPProtocol::ParseIncomplete:
			DEBUG(log_) << "Could not completely parse input, waiting for more.";
			return;
		case HTTPProtocol::ParseFailure:
			send_response(HTTPProtocol::BadRequest, "Could not parse input line.");
			return;
		}

		if (state_ == GetStart) {
			ASSERT(request_.start_line_.empty());
			if (line.empty()) {
				ERROR(log_) << "Premature end of headers.";
				send_response(HTTPProtocol::BadRequest, "Empty start line.");
				return;
			}
			request_.start_line_ = line;

			/*
			 * There are two kinds of request line.  The first has two
			 * words, the second has three.  Anything else is malformed.
			 *
			 * The first kind is HTTP/0.9.  The second kind can be
			 * anything, especially HTTP/1.0 and HTTP/1.1.
			 */
			std::vector<Buffer> words = line.split(' ', false);
			if (words.empty()) {
				send_response(HTTPProtocol::BadRequest, "Empty start line.");
				return;
			}

			if (words.size() == 3) {
				/*
				 * HTTP/1.0 or HTTP/1.1.  Get headers.
				 */
				state_ = GetHeaders;

				if (buffer_.empty())
					break;
				continue;
			}

			if (words.size() != 2) {
				/*
				 * Not HTTP/0.9.
				 */
				send_response(HTTPProtocol::BadRequest, "Too many request parameters.");
				return;
			}

			/*
			 * HTTP/0.9.  This is all we should get from the client.
			 */
			if (!buffer_.empty()) {
				send_response(HTTPProtocol::BadRequest, "Garbage after HTTP/0.9-style request.");
				return;
			}

			/*
			 * We have received the full message.  Process any
			 * pending callback.
			 */
			state_ = GotRequest;

			ASSERT(action_ == NULL);
			if (callback_ != NULL) {
				action_ = schedule_callback(callback_);
				callback_ = NULL;
			}
			return;
		}

		ASSERT(state_ == GetHeaders);
		ASSERT(!request_.start_line_.empty());

		/*
		 * Process end of headers!
		 */
		if (line.empty()) {
			if (!buffer_.empty()) {
				ERROR(log_) << "Client sent garbage after message.";
				send_response(HTTPProtocol::BadRequest, "Garbage after message.");
				return;
			}

			/*
			 * We have received the full message.  Process any
			 * pending callback.
			 */
			state_ = GotRequest;

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
				send_response(HTTPProtocol::BadRequest, "Folded header sent before any others.");
				return;
			}

			request_.headers_[last_header_].back().append(line);
			if (buffer_.empty())
				break;
			continue;
		}

		unsigned pos;
		if (!line.find(':', &pos)) {
			send_response(HTTPProtocol::BadRequest, "Empty header name.");
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

		request_.headers_[header].push_back(value);
		last_header_ = header;

		if (buffer_.empty())
			break;
	}
}
