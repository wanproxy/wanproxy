#ifndef	SSH_KEY_EXCHANGE_H
#define	SSH_KEY_EXCHANGE_H

class SSHKeyExchange {
	std::string name_;
protected:
	SSHKeyExchange(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHKeyExchange()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

#endif /* !SSH_KEY_EXCHANGE_H */
