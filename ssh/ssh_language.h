#ifndef	SSH_SSH_LANGUAGE_H
#define	SSH_SSH_LANGUAGE_H

namespace SSH {
	class Language {
		std::string name_;
	protected:
		Language(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~Language()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual Language *clone(void) const = 0;

		virtual bool input(Buffer *) = 0;
	};
}

#endif /* !SSH_SSH_LANGUAGE_H */
