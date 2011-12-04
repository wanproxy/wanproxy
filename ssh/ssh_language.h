#ifndef	SSH_LANGUAGE_H
#define	SSH_LANGUAGE_H

class SSHLanguage {
	std::string name_;
protected:
	SSHLanguage(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHLanguage()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

#endif /* !SSH_LANGUAGE_H */
