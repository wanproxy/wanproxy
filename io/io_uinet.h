/*
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
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

#ifndef	IO_IO_UINET_H
#define	IO_IO_UINET_H


#include <event/callback_thread.h>

#include <uinet_api.h>

class CallbackScheduler;
class CallbackThread;

class IOUinet {
	LogHandle log_;
	CallbackThread *handler_thread_;

	IOUinet(void);
	~IOUinet();

public:
	int add_interface(uinet_iftype_t, const std::string&, const std::string&, unsigned int, int);
	int interface_up(const std::string&, bool);
	int remove_interface(const std::string&);

	CallbackScheduler *scheduler(void) const { return handler_thread_; }

	void start(bool enable_lo0);

	/*
	 * XXX doesn't this need to be MT-safe? (ditto IOSystem::instance)
	 */
	static IOUinet *instance(void)
	{
		static IOUinet *instance_;

		if (instance_ == NULL)
			instance_ = new IOUinet();
		return (instance_);
	}
};

#endif /* !IO_IO_UINET_H */
