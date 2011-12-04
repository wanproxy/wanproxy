#ifndef	SSH_MAC_H
#define	SSH_MAC_H

class SSHMAC {
	std::string name_;
protected:
	SSHMAC(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHMAC()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

#endif /* !SSH_MAC_H */
