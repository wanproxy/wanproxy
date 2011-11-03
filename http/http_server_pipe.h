#ifndef	HTTP_SERVER_PIPE_H
#define	HTTP_SERVER_PIPE_H

#include <event/event.h>
#include <event/typed_pair_callback.h>

#include <http/http_protocol.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

typedef class TypedPairCallback<Event, HTTPProtocol::Message> HTTPMessageEventCallback;

class HTTPServerPipe : public PipeProducer {
	enum State {
		GetStart,
		GetHeaders,

		GotMessage,
		Error
	};

	State state_;
	Buffer buffer_;

	HTTPProtocol::Message message_;
	std::string last_header_;

	Action *action_;
	HTTPMessageEventCallback *callback_;
public:
	HTTPServerPipe(const LogHandle&);
	~HTTPServerPipe();

	Action *message(HTTPMessageEventCallback *);

	/* XXX Use HTTPProtocol::Message type and add nice constructors for it?  */
	void send_response(HTTPProtocol::Status, Buffer *, Buffer *);
	void send_response(HTTPProtocol::Status, const std::string&, const std::string& = "text/plain");

private:
	void cancel(void);

	Action *schedule_callback(HTTPMessageEventCallback *);

	void consume(Buffer *);
};

#endif /* !HTTP_SERVER_PIPE_H */
