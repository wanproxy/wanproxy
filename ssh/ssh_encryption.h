#ifndef	SSH_ENCRYPTION_H
#define	SSH_ENCRYPTION_H

class SSHEncryption {
	std::string name_;
protected:
	SSHEncryption(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHEncryption()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

#endif /* !SSH_ENCRYPTION_H */
