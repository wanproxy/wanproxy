#ifndef	ATOMIC_H
#define	ATOMIC_H

template<typename T>
class Atomic {
	T val_;
public:
	Atomic(void)
	: val_()
	{ }

	template<typename Ta>
	Atomic(Ta arg)
	: val_(arg)
	{ }

	~Atomic()
	{ }

	/*
	 * Note that these are deliberately not operator overloads, to force
	 * deliberate use of this class.
	 */

#if defined(__GNUC__)
	template<typename Ta>
	T add(Ta arg)
	{
		return (__sync_fetch_and_add(&val_, arg));
	}

	template<typename Ta>
	T subtract(Ta arg)
	{
		return (__sync_fetch_and_sub(&val_, arg));
	}

	template<typename Ta>
	T set(Ta arg)
	{
		return (__sync_fetch_and_or(&val_, arg));
	}

	template<typename Ta>
	T mask(Ta arg)
	{
		return (__sync_fetch_and_and(&val_, arg));
	}

	template<typename Ta>
	T clear(Ta arg)
	{
		return (__sync_fetch_and_and(&val_, ~arg));
	}

	template<typename To, typename Tn>
	T cmpset(Ta oldval, Tn newval)
	{
		return (__sync_val_compare_and_swap(&val_, oldval, newval));
	}
#else
#error "No support for atomic operations for your compiler.  Why not add some?"
#endif
};

#endif /* !ATOMIC_H */
