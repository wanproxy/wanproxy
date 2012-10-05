#ifndef	SSH_SSH_SERVER_HOST_KEY_H
#define	SSH_SSH_SERVER_HOST_KEY_H

class Buffer;

namespace SSH {
	struct Session;

	class ServerHostKey {
		std::string name_;
	protected:
		ServerHostKey(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~ServerHostKey()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual ServerHostKey *clone(void) const = 0;

		virtual bool decode_public_key(Buffer *) = 0;
		virtual void encode_public_key(Buffer *) const = 0;

		virtual bool sign(Buffer *, const Buffer *) const = 0;
		virtual bool verify(const Buffer *, const Buffer *) const = 0;

		static void add_client_algorithms(Session *);
		static ServerHostKey *server(Session *, const std::string&);
	};
}

#endif /* !SSH_SSH_SERVER_HOST_KEY_H */
