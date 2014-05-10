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

#ifndef	COMMON_REF_H
#define	COMMON_REF_H

template<typename T>
class Ref {
	class RefObj {
		T *ptr_;
		unsigned count_;
	public:
		RefObj(T *ptr)
		: ptr_(ptr),
		  count_(1)
		{ }

	private:
		~RefObj()
		{
			delete ptr_;
			ptr_ = NULL;
		}

	public:
		void hold(void)
		{
			count_++;
		}

		void drop(void)
		{
			if (count_-- == 1)
				delete this;
		}

		bool exclusive(void) const
		{
			return (count_ == 1);
		}

		T *get(void) const
		{
			return (ptr_);
		}
	};

	RefObj *obj_;
public:
	Ref(void)
	: obj_(NULL)
	{ }

	Ref(T *ptr)
	: obj_(NULL)
	{
		if (ptr != NULL)
			obj_ = new RefObj(ptr);
	}

	/*
	 * XXX Template...  See operator=.
	 */
	Ref(const Ref& ref)
	: obj_(ref.obj_)
	{
		if (obj_ != NULL)
			obj_->hold();
	}

	~Ref()
	{
		if (obj_ != NULL) {
			obj_->drop();
			obj_ = NULL;
		}
	}

	/*
	 * XXX
	 * Template so we can take a pointer from a Ref<> with a compatible
	 * base class?
	 */
	const Ref& operator= (const Ref& ref)
	{
		if (obj_ != NULL) {
			obj_->drop();
			obj_ = NULL;
		}

		if (ref.obj_ != NULL) {
			obj_ = ref.obj_;
			obj_->hold();
		}
		return (*this);
	}

	T *operator-> (void) const
	{
		return (obj_->get());
	}

	const T& operator* (void) const
	{
		const T *ptr = obj_->get();
		return (*ptr);
	}

	template<typename Tc>
	Tc cast(void) const
	{
		const T *ptr = obj_->get();
		return (dynamic_cast<Tc>(ptr));
	}

	bool exclusive(void) const
	{
		return (obj_->exclusive());
	}

	bool null(void) const
	{
		return (obj_ == NULL);
	}

	bool operator< (const Ref& b) const
	{
		return (obj_ < b.obj_);
	}
};

#endif /* !COMMON_REF_H */
