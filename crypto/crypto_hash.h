#ifndef	CRYPTO_CRYPTO_HASH_H
#define	CRYPTO_CRYPTO_HASH_H

#include <set>

#include <event/event_callback.h>

namespace CryptoHash {
	class Method;

	enum Algorithm {
		MD5,
		SHA1,
		SHA256,
	};

	class Instance {
	protected:
		Instance(void)
		{ }

	public:
		virtual ~Instance()
		{ }

		virtual bool hash(Buffer *, const Buffer *) = 0;

		virtual Action *submit(Buffer *, EventCallback *) = 0;
	};

	class Method {
		std::string name_;
	protected:
		Method(const std::string&);

		virtual ~Method()
		{ }
	public:
		virtual std::set<Algorithm> algorithms(void) const = 0;
		virtual Instance *instance(Algorithm) const = 0;

		static const Method *method(Algorithm);
	};

	static inline Instance *instance(Algorithm algorithm)
	{
		const Method *method = Method::method(algorithm);
		if (method == NULL)
			return (NULL);
		return (method->instance(algorithm));
	}

	static inline bool hash(Algorithm algorithm, Buffer *out, const Buffer *in)
	{
		Instance *i = instance(algorithm);
		if (i == NULL)
			return (false);
		bool ok = i->hash(out, in);
		delete i;
		return (ok);
	}
}

std::ostream& operator<< (std::ostream&, CryptoHash::Algorithm);

#endif /* !CRYPTO_CRYPTO_HASH_H */
