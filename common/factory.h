#ifndef	FACTORY_H
#define	FACTORY_H

#include <map>

template<class C>
class Factory {
public:
	Factory(void)
	{ }

	virtual ~Factory()
	{ }

	virtual C *create(void) const = 0;
};

template<class B, class C>
class ConstructorFactory : public Factory<B> {
public:
	ConstructorFactory(void)
	{ }

	~ConstructorFactory()
	{ }

	B *create(void) const
	{
		return (new C());
	}
};

template<class B, class C, typename A>
class ConstructorArgFactory : public Factory<C> {
	A a_;
public:
	ConstructorArgFactory(A a)
	: a_(a)
	{ }

	~ConstructorArgFactory()
	{ }

	B *create(void) const
	{
		return (new C(a_));
	}
};

template<class B, class C>
class SubclassFactory : public Factory<B> {
	Factory<C> *factory_;
public:
	SubclassFactory(Factory<C> *factory)
	: factory_(factory)
	{ }

	~SubclassFactory()
	{
		delete factory_;
		factory_ = NULL;
	}

	B *create(void) const
	{
		return (factory_->create());
	}
};

template<class C>
struct factory {
	Factory<C> *operator() (void) const
	{
		return (new ConstructorFactory<C, C>);
	}

	template<typename T>
	Factory<C> *operator() (T arg) const
	{
		return (new ConstructorArgFactory<C, C, T>(arg));
	}
};

template<typename K, typename C>
class FactoryMap {
	typedef std::map<K, Factory<C> *> map_type;
	map_type map_;
public:
	FactoryMap(void)
	{ }

	~FactoryMap()
	{
		typename map_type::iterator it;

		while ((it = map_.begin()) != map_.end()) {
			delete it->second;
			map_.erase(it);
		}
	}

	C *create(const K& key) const
	{
		typename map_type::const_iterator it;

		it = map_.find(key);
		if (it == map_.end())
			return (NULL);
		return (it->second->create());
	}

	void enter(const K& key, Factory<C> *factory)
	{
		typename map_type::iterator it;

		it = map_.find(key);
		if (it != map_.end()) {
			delete it->second;
			map_.erase(it);
		}

		map_[key] = factory;
	}

	template<typename S>
	void enter(const K& key, Factory<S> *factory)
	{
		enter(key, new SubclassFactory<C, S>(factory));
	}
};

#endif /* !FACTORY_H */
