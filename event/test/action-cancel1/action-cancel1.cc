/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#include <common/test.h>

#include <event/action.h>

struct Cancelled {
	Test test_;
	bool cancelled_;

	Cancelled(TestGroup& g)
	: test_(g, "Cancellation does occur."),
	  cancelled_(false)
	{
		Action *a = cancellation(this, &Cancelled::cancel);
		a->cancel();
	}

	~Cancelled()
	{
		if (cancelled_)
			test_.pass();
	}

	void cancel(void)
	{
		ASSERT("/cancelled", !cancelled_);
		cancelled_ = true;
	}
};

struct NotCancelled {
	Test test_;
	bool cancelled_;
	Action *action_;

	NotCancelled(TestGroup& g)
	: test_(g, "Cancellation does not occur."),
	  cancelled_(false),
	  action_(NULL)
	{
		action_ = cancellation(this, &NotCancelled::cancel);
	}

	~NotCancelled()
	{
		if (!cancelled_) {
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;
				ASSERT("/not/cancelled", cancelled_);
			}
		}
	}

	void cancel(void)
	{
		ASSERT("/not/cancelled", !cancelled_);
		cancelled_ = true;
		test_.pass();
	}
};

int
main(void)
{
	TestGroup g("/test/action/cancel1", "Action::cancel #1");
	{
		Cancelled _(g);
	}
	{
		NotCancelled _(g);
	}
}
