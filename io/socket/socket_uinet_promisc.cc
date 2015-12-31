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

#include <event/event_callback.h>

#include <io/socket/socket_uinet_promisc.h>

#include <uinet_api.h>

SocketUinetPromisc::SocketUinetPromisc(struct uinet_socket *so, int domain, int socktype, int protocol)
: SocketUinet(so, domain, socktype, protocol),
  log_("/socket/uinet/promisc"),
  cdom_(0),
  synfilter_callback_(NULL)
{
}

SocketUinetPromisc::~SocketUinetPromisc()
{
}

void
SocketUinetPromisc::getconninfo(struct uinet_in_conninfo *inc)
{
	uinet_sogetconninfo(so_, inc);
}

int
SocketUinetPromisc::getl2info(struct uinet_in_l2info *l2i)
{
	int error = uinet_getl2info(so_, l2i);
	return (uinet_errno_to_os(error));
}

int
SocketUinetPromisc::setl2info(const struct uinet_in_l2info *l2i)
{
	int error = uinet_setl2info(so_, l2i);
	return (uinet_errno_to_os(error));
}

int
SocketUinetPromisc::setl2info2(const uint8_t *laddr, const uint8_t *faddr, uint16_t flags, const struct uinet_in_l2tagstack *tagstack)
{
	int error = uinet_setl2info2(so_, laddr, faddr, flags, tagstack);
	return (uinet_errno_to_os(error));
}

int
SocketUinetPromisc::setsynfilter(SynfilterCallback *cb)
{
	ASSERT_NULL(log_, synfilter_callback_);

	synfilter_callback_ = cb;

	int error = uinet_synfilter_install(so_, synfilter_callback, this);
	
	return (uinet_errno_to_os(error));
}

int
synfilter_callback(struct uinet_socket *listener, void *arg, uinet_api_synfilter_cookie_t cookie)
{
	SocketUinetPromisc *s = static_cast<SocketUinetPromisc *>(arg);
	struct uinet_in_conninfo inc;
	struct uinet_in_l2info l2i;
	SocketUinetPromisc::SynfilterCallbackParam info;

	(void)listener;

	ASSERT_NON_NULL(s->log_, s->synfilter_callback_);

	uinet_synfilter_getconninfo(cookie, &inc);
	uinet_synfilter_getl2info(cookie, &l2i);

	int synfilter_decision = UINET_SYNF_REJECT_SILENT;

	info.inc = &inc;
	info.l2i = &l2i;
	info.cookie = cookie;
	info.result = &synfilter_decision;

	s->synfilter_callback_->param(info);
	s->synfilter_callback_->execute();

	return (synfilter_decision);
}

uinet_synf_deferral_t
SocketUinetPromisc::synfdefer(uinet_api_synfilter_cookie_t cookie)
{
	return (uinet_synfilter_deferral_alloc(so_, cookie));
}

int
SocketUinetPromisc::synfdeferraldeliver(uinet_synf_deferral_t deferral, int decision)
{
	int error = uinet_synfilter_deferral_deliver(so_, deferral, decision);
	return (uinet_errno_to_os(error));
}

SocketUinetPromisc *
SocketUinetPromisc::create(SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint, unsigned int cdom)
{
	SocketUinetPromisc *s = create_basic<SocketUinetPromisc>(family, type, protocol, hint);
	if (NULL == s)
		return (NULL);

	s->cdom_ = cdom;

	int error = uinet_make_socket_promiscuous(s->so_, s->cdom_);
	if (error != 0) {
		ERROR("/socket/uinet/promisc") << "Failed to make socket promiscuous: " << strerror(uinet_errno_to_os(error));
		/* XXX I believe this leaks the underlying socket */
		delete s;
		s = NULL;
	}

	return (s);
}
