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

#ifndef	EVENT_DROPBOX_H
#define	EVENT_DROPBOX_H

#include <event/typed_condition.h>

template<typename T>
class Dropbox {
	T item_;
	bool item_dropped_;
	TypedConditionVariable<T> item_condition_;
public:
	Dropbox(void)
	: item_(),
	  item_dropped_(false),
	  item_condition_()
	{ }

	~Dropbox()
	{ }

	void clear(void)
	{
		item_dropped_ = false;
	}

	Action *get(TypedCallback<T> *cb)
	{
		Action *a = item_condition_.wait(cb);
		if (item_dropped_)
			item_condition_.signal(item_);
		return (a);
	}

	void put(T item)
	{
		if (item_dropped_)
			DEBUG("/dropbox") << "Dropbox contents replaced.";
		item_ = item;
		item_dropped_ = true;
		item_condition_.signal(item_);
	}
};

#endif /* !EVENT_DROPBOX_H */
