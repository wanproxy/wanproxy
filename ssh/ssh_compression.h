#ifndef	SSH_COMPRESSION_H
#define	SSH_COMPRESSION_H

class SSHCompression {
	std::string name_;
protected:
	SSHCompression(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHCompression()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

#endif /* !SSH_COMPRESSION_H */
