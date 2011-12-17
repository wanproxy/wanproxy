#ifndef	SSH_MAC_H
#define	SSH_MAC_H

namespace SSH {
	class MAC {
		std::string name_;
	protected:
		MAC(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~MAC()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;
	};
}

#endif /* !SSH_MAC_H */
