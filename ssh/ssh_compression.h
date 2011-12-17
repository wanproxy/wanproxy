#ifndef	SSH_COMPRESSION_H
#define	SSH_COMPRESSION_H

namespace SSH {
	class Compression {
		std::string name_;
	protected:
		Compression(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~Compression()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;
	};
}

#endif /* !SSH_COMPRESSION_H */
