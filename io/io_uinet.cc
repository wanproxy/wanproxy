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


#include <event/callback_thread.h>
#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/io_uinet.h>

#include <uinet_api.h>

IOUinet::IOUinet(void)
	: log_("/io/uinet"),
	  handler_thread_(new CallbackThread("UINET IOThread"))
{
}


IOUinet::~IOUinet()
{
}


int
IOUinet::add_interface(uinet_iftype_t type, const std::string& configstr, const std::string& alias, unsigned int cdom, int cpu)
{
	int error;

	error = uinet_ifcreate(type, configstr.c_str(), alias.c_str(), cdom, cpu, NULL);
	return (uinet_errno_to_os(error));
}


int
IOUinet::remove_interface(const std::string& name)
{
	int error;

	error = uinet_ifdestroy_byname(name.c_str());
	return (uinet_errno_to_os(error));
}


int
IOUinet::interface_up(const std::string& name, bool ispromisc)
{
	int error;

	error = uinet_interface_up(name.c_str(), ispromisc);
	return (uinet_errno_to_os(error));
}


void
IOUinet::start(bool enable_lo0)
{
	/*
	 * Prepare system to handle UINET IO.
	 */
	INFO(log_) << "Starting UINET IO system.";

	/*
	 * XXX number of CPUs and limit on number of mbuf clusters should be
	 * configurable
	 */
	uinet_init(1, 128*1024, enable_lo0);

	handler_thread_->start();

	EventSystem::instance()->thread_wait(handler_thread_);
}
