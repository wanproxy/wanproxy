#ifndef	SSH_SSH_SERVER_HOST_KEY_H
#define	SSH_SSH_SERVER_HOST_KEY_H

class Buffer;

namespace SSH {
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

		virtual void encode_public_key(Buffer *) const = 0;

		virtual bool sign(Buffer *, const Buffer *) const = 0;

		static ServerHostKey *server(const std::string&);
	};
}

#endif /* !SSH_SSH_SERVER_HOST_KEY_H */
