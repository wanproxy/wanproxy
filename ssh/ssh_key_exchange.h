#ifndef	SSH_SSH_KEY_EXCHANGE_H
#define	SSH_SSH_KEY_EXCHANGE_H

class Buffer;

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

		static KeyExchange *method(void);
	};
}

#endif /* !SSH_SSH_KEY_EXCHANGE_H */
