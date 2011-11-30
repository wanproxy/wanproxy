#ifndef	HTTP_PROTOCOL_H
#define	HTTP_PROTOCOL_H

#include <map>

namespace HTTPProtocol {
	enum ParseStatus {
		ParseSuccess,
		ParseFailure,
		ParseIncomplete,
	};

	struct Message {
		enum Type {
			Request,
			Response,
		};

		Type type_;
		Buffer start_line_;
		std::map<std::string, std::vector<Buffer> > headers_;
		Buffer body_;
#if 0
		std::map<std::string, std::vector<Buffer> > trailers_;
#endif

		Message(Type type)
		: type_(type),
		  start_line_(),
		  headers_(),
		  body_()
		{ }

		~Message()
		{ }

		bool decode(Buffer *);
	};

	struct Request : public Message {
		Request(void)
		: Message(Message::Request)
		{ }

		~Request()
		{ }
	};

	struct Response : public Message {
		Response(void)
		: Message(Message::Response)
		{ }

		~Response()
		{ }
	};

	enum Status {
		OK,
		BadRequest,
		NotFound,
		NotImplemented,
		VersionNotSupported,
	};

	bool DecodeURI(Buffer *, Buffer *);
	ParseStatus ExtractLine(Buffer *, Buffer *);
}

#endif /* !HTTP_PROTOCOL_H */
