/*
 * Copyright (c) 2015 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_DESTROY_THREAD_H
#define	EVENT_DESTROY_THREAD_H

/*
 * This allows deletes to be queued in a specific
 * thread once an object is ready for garbage
 * collection, but with the requirement for a given
 * mutex to be held before it can be deleted.
 *
 * You should not use this.  There should not be
 * free-floating objects which delete themselves,
 * but they should be managed by the objects that
 * create them.  Still, having allowed this idiom
 * to creep into the codebase in small amounts, it
 * must be made at least semi-usable.
 */

#include <deque>

#include <common/thread/thread.h>

class DestroyThread : public Thread {
	class DestroyBase {
		friend class DestroyThread;

		Lock *lock_;
	protected:
		DestroyBase(Lock *lock)
		: lock_(lock)
		{ }

	private:
		virtual ~DestroyBase()
		{ }

		virtual void *obj(void) const = 0;
	};

	template<class C>
	class DestroyObject : public DestroyBase {
		C *obj_;
	public:
		DestroyObject(Lock *lock, C *xobj)
		: DestroyBase(lock),
		  obj_(xobj)
		{ }

	private:
		~DestroyObject()
		{
			delete obj_;
			obj_ = NULL;
		}

		void *obj(void) const
		{
			return ((void *)obj_);
		}
	};

	LogHandle log_;
	Mutex mtx_;
	SleepQueue sleepq_;
	bool idle_;
	std::deque<DestroyBase *> queue_;
public:
	DestroyThread(void);

	~DestroyThread()
	{ }

	template<class C>
	void destroy(Lock *lock, C *obj)
	{
		append(new DestroyObject<C>(lock, obj));
	}

private:
	void append(DestroyBase *);

	void main(void);

public:
	virtual void stop(void)
	{
		ScopedLock _(&mtx_);
		if (stop_)
			return;
		stop_ = true;
		sleepq_.signal();
	}
};

#endif /* !EVENT_DESTROY_THREAD_H */
