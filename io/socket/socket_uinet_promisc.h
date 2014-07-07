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

#ifndef	IO_SOCKET_SOCKET_UINET_PROMISC_H
#define	IO_SOCKET_SOCKET_UINET_PROMISC_H

#include <event/typed_callback.h>

#include <io/socket/socket_uinet.h>

extern "C" {
	int synfilter_callback(struct uinet_socket *, void *, uinet_api_synfilter_cookie_t);	
}

class SocketUinetPromisc : public SocketUinet {
public:
	struct SynfilterCallbackParam {
		struct uinet_in_conninfo *inc;
		struct uinet_in_l2info *l2i;
		uinet_api_synfilter_cookie_t cookie;
		int *result;
	};

	typedef	class TypedCallback<SynfilterCallbackParam> SynfilterCallback;

private:
	friend class SocketUinet; /* So create_basic can invoke the constructor. */
	friend int synfilter_callback(struct uinet_socket *, void *, uinet_api_synfilter_cookie_t);

	LogHandle log_;
	unsigned int cdom_;
	SynfilterCallback *synfilter_callback_;

protected:
	SocketUinetPromisc(struct uinet_socket *, int, int, int);

public:
	~SocketUinetPromisc();

	void getconninfo(struct uinet_in_conninfo *inc);
	int getl2info(struct uinet_in_l2info *);
	int setl2info(const struct uinet_in_l2info *);
	int setl2info2(const uint8_t *laddr, const uint8_t *faddr, uint16_t flags, const struct uinet_in_l2tagstack *tagstack);
	int setsynfilter(SynfilterCallback *);
	
	uinet_synf_deferral_t synfdefer(uinet_api_synfilter_cookie_t);
	int synfdeferraldeliver(uinet_synf_deferral_t, int);

public:
	static SocketUinetPromisc *create(SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "", unsigned int = 0);
};

#endif /* !IO_SOCKET_SOCKET_UINET_PROMISC_H */
