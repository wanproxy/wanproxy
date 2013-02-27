/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	COMMON_FACTORY_H
#define	COMMON_FACTORY_H

#include <map>
#include <set>

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
class ConstructorArgFactory : public Factory<B> {
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

template<class B, class C = B>
struct factory {
	Factory<B> *operator() (void) const
	{
		return (new ConstructorFactory<B, C>);
	}

	template<typename T>
	Factory<B> *operator() (T arg) const
	{
		return (new ConstructorArgFactory<B, C, T>(arg));
	}
};

template<typename K, typename C>
class FactoryMap {
	typedef std::map<K, Factory<C> *> map_type;

	map_type map_;
public:
	FactoryMap(void)
	: map_()
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

	std::set<K> keys(void) const
	{
		typename map_type::const_iterator it;
		std::set<K> key_set;

		for (it = map_.begin(); it != map_.end(); ++it)
			key_set.insert(it->first);

		return (key_set);
	}
};

#endif /* !COMMON_FACTORY_H */
