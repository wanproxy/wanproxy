#ifndef	SSH_SSH_SERVER_HOST_KEY_H
#define	SSH_SSH_SERVER_HOST_KEY_H

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

		virtual bool input(Buffer *) = 0;
	};
}

#endif /* !SSH_SSH_SERVER_HOST_KEY_H */
