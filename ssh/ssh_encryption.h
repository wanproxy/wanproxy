#ifndef	SSH_ENCRYPTION_H
#define	SSH_ENCRYPTION_H

namespace SSH {
	class Encryption {
		std::string name_;
	protected:
		Encryption(const std::string& xname)
		: name_(xname)
		{ }

	public:
		~Encryption()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;
	};
}

#endif /* !SSH_ENCRYPTION_H */
