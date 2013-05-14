/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

class ReloadTest {
	Action *reload_action_;
	unsigned reload_count_;
public:
	ReloadTest(void)
	: reload_action_(NULL),
	  reload_count_(0)
	{
		reload_action_ = EventSystem::instance()->register_interest(EventInterestReload, callback(this, &ReloadTest::reload));
	}

	~ReloadTest()
	{
		if (reload_action_ != NULL) {
			reload_action_->cancel();
			reload_action_ = NULL;
		}
	}

private:
	void reload(void)
	{
		reload_action_->cancel();
		reload_action_ = NULL;

		INFO("/event/reload/test1") << "Reload #" << ++reload_count_;

		reload_action_ = EventSystem::instance()->register_interest(EventInterestReload, callback(this, &ReloadTest::reload));
	}
};

int
main(void)
{
	ReloadTest *tt = new ReloadTest();

	event_main();

	delete tt;
}
