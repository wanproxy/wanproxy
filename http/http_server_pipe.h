#ifndef	HTTP_HTTP_SERVER_PIPE_H
#define	HTTP_HTTP_SERVER_PIPE_H

#include <event/event.h>
#include <event/typed_pair_callback.h>

#include <http/http_protocol.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

typedef class TypedPairCallback<Event, HTTPProtocol::Request> HTTPRequestEventCallback;

class HTTPServerPipe : public PipeProducer {
	enum State {
		GetStart,
		GetHeaders,

		GotRequest,
		Error
	};

	State state_;
	Buffer buffer_;

	HTTPProtocol::Request request_;
	std::string last_header_;

	Action *action_;
	HTTPRequestEventCallback *callback_;
public:
	HTTPServerPipe(const LogHandle&);
	~HTTPServerPipe();

	Action *request(HTTPRequestEventCallback *);

	/* XXX Use HTTPProtocol::Response type and add nice constructors for it?  */
	void send_response(HTTPProtocol::Status, Buffer *, Buffer *);
	void send_response(HTTPProtocol::Status, const std::string&, const std::string& = "text/plain");

private:
	void cancel(void);

	Action *schedule_callback(HTTPRequestEventCallback *);

	void consume(Buffer *);
};

#endif /* !HTTP_HTTP_SERVER_PIPE_H */
