#ifndef	HTTP_PROTOCOL_H
#define	HTTP_PROTOCOL_H

namespace HTTPProtocol {
	struct Message {
		Buffer start_line_;
		std::map<std::string, std::vector<Buffer> > headers_;
#if 0
		Buffer body_;
		std::map<std::string, std::vector<Buffer> > trailers_;
#endif
	};

	enum Status {
		OK,
		BadRequest,
		NotFound,
		NotImplemented,
		VersionNotSupported,
	};
}

#endif /* !HTTP_PROTOCOL_H */
