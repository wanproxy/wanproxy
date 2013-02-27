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

#include <crypto/crypto_random.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

class RandomSpeed {
	CryptoRandomSession *session_;
	Buffer buffer_;
	uintmax_t bytes_;
	Action *callback_action_;
	Action *timeout_action_;
public:
	RandomSpeed(CryptoRandomType type)
	: session_(NULL),
	  bytes_(0),
	  callback_action_(NULL),
	  timeout_action_(NULL)
	{
		session_ = CryptoRandomMethod::default_method->session(type);

		callback_action_ = callback(this, &RandomSpeed::callback_complete)->schedule();

		INFO("/example/prng/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &RandomSpeed::timer));
	}

	~RandomSpeed()
	{
		ASSERT("/example/prng/speed1", timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		EventCallback *cb = callback(this, &RandomSpeed::session_callback);
		callback_action_ = session_->generate(8192, cb);
	}

	void session_callback(Event e)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		ASSERT("/example/prng/speed1", e.type_ == Event::Done);
		bytes_ += e.buffer_.length();

		callback_action_ = callback(this, &RandomSpeed::callback_complete)->schedule();
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		ASSERT("/example/prng/speed1", callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;

		INFO("/example/prng/speed1") << "Timer expired; " << bytes_ << " bytes generated.";
	}
};

int
main(void)
{
	INFO("/example/prng/speed1") << "Timer delay: " << TIMER_MS << "ms";

	RandomSpeed _(CryptoTypePRNG);

	event_main();
}
