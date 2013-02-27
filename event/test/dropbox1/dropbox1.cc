/*
 * Copyright (c) 2010 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>
#include <common/test.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/dropbox.h>
#include <event/event_main.h>

struct DropboxTest {
	unsigned key_;
	TestGroup& group_;
	Test test_;
	Dropbox<unsigned> dropbox_;
	Action *action_;

	DropboxTest(unsigned key, TestGroup& group)
	: key_(key),
	  group_(group),
	  test_(group_, "Dropbox is used."),
	  dropbox_(),
	  action_(NULL)
	{
		TypedCallback<unsigned> *cb = callback(this, &DropboxTest::item_dropped);
		if (key_ % 2 == 0) {
			action_ = dropbox_.get(cb);
			dropbox_.put(key_);
		} else {
			dropbox_.put(key_);
			action_ = dropbox_.get(cb);
		}
	}

	~DropboxTest()
	{
		{
			Test _(group_, "No outstanding action.");
			if (action_ == NULL)
				_.pass();
		}
	}

	void item_dropped(unsigned key)
	{
		test_.pass();
		{
			Test _(group_, "Outstanding action.");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}
		{
			Test _(group_, "Correct value.");
			if (key == key_)
				_.pass();
		}

		delete this;
	}
};

int
main(void)
{
	TestGroup g("/test/dropbox1", "Dropbox #1");

	unsigned i;
	for (i = 0; i < 100; i++) {
		new DropboxTest(i, g);
	}

	event_main();
}
