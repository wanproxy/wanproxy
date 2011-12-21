#ifndef	COMMON_REGISTRAR_H
#define	COMMON_REGISTRAR_H

#include <set>

/*
 * U is merely a type used as a unique key, so that multiple Registrars of
 * objects with the same type can coexist.
 */
template<typename U, typename T>
class Registrar {
	std::set<T> registered_set_;

	Registrar(void)
	: registered_set_()
	{ }

	~Registrar()
	{ }
public:
	void enter(T item)
	{
		registered_set_.insert(item);
	}

	std::set<T> enumerate(void) const
	{
		return (registered_set_);
	}

	static Registrar *instance(void)
	{
		static Registrar *instance;

		if (instance == NULL)
			instance = new Registrar();
		return (instance);
	}
};

#endif /* !COMMON_REGISTRAR_H */
