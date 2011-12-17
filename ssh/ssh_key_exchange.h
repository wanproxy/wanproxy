#ifndef	SSH_KEY_EXCHANGE_H
#define	SSH_KEY_EXCHANGE_H

namespace SSH {
	class KeyExchange {
		std::string name_;
	protected:
		KeyExchange(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~KeyExchange()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;
	};
}

#endif /* !SSH_KEY_EXCHANGE_H */
