#ifndef	HTTP_SERVER_PIPE_H
#define	HTTP_SERVER_PIPE_H

#include <event/event.h>
#include <event/typed_pair_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

struct HTTPMessage {
	Buffer start_line_;
	std::map<std::string, std::vector<Buffer> > headers_;
#if 0
	Buffer body_;
	std::map<std::string, std::vector<Buffer> > trailers_;
#endif
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
	HTTPServerPipe(const LogHandle&);
	~HTTPServerPipe();

	Action *message(HTTPMessageEventCallback *);

	/* XXX Use HTTPMessage type and add nice constructors for it?  */
	void send_response(Status, Buffer *, Buffer *);
	void send_response(Status, const std::string&, const std::string& = "text/plain");

private:
	void cancel(void);

	Action *schedule_callback(HTTPMessageEventCallback *);

	void consume(Buffer *);
};

#endif /* !HTTP_SERVER_PIPE_H */
