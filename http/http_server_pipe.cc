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

HTTPServerPipe::HTTPServerPipe(const LogHandle& log)
: PipeProducer(log),
  state_(GetStart),
  buffer_(),
  message_(),
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
HTTPServerPipe::message(HTTPMessageEventCallback *cb)
{
	ASSERT(action_ == NULL);
	ASSERT(callback_ == NULL);

	if (state_ == GotMessage || state_ == Error)
		return (schedule_callback(cb));

	callback_ = cb;
	return (cancellation(this, &HTTPServerPipe::cancel));
}

void
HTTPServerPipe::send_response(Status status, Buffer *body, Buffer *headers)
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

void
HTTPServerPipe::send_response(Status status, const std::string& body, const std::string& content_type)
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
HTTPServerPipe::schedule_callback(HTTPMessageEventCallback *cb)
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

void
HTTPServerPipe::consume(Buffer *in)
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

			/*
			 * XXX
			 * If we're really doing HTTP and not just something faux HTTP,
			 * we need to handle HTTP/0.9-style requests here.
			 */

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
