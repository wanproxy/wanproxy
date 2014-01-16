/*
 * Copyright (c) 2008-2012 Juli Mallett. All rights reserved.
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



#include <netdb.h>
#include <stdio.h>

#include <io/socket/resolver.h>


/*
 * XXX Presently using AF_INET6 as the test for what is supported, but that is
 * wrong in many, many ways.
 */


#if defined(__OPENNT)
struct addrinfo {
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	struct sockaddr *ai_addr;
	socklen_t ai_addrlen;

	struct sockaddr_in _ai_inet;
};

static int
getaddrinfo(const char *host, const char *serv, const struct addrinfo *hints,
	    struct addrinfo **aip)
{
	struct addrinfo *ai;
	struct hostent *he;

	he = gethostbyname(host);
	if (he == NULL)
		return (1);

	ai = new addrinfo;
	ai->ai_family = AF_INET;
	ai->ai_socktype = hints->ai_socktype;
	ai->ai_protocol = hints->ai_protocol;
	ai->ai_addr = (struct sockaddr *)&ai->_ai_inet;
	ai->ai_addrlen = sizeof ai->_ai_inet;

	memset(&ai->_ai_inet, 0, sizeof ai->_ai_inet);
	ai->_ai_inet.sin_family = AF_INET;
	/* XXX _ai_inet.sin_addr on non-__OPENNT */

	ASSERT(log_, he->h_length == sizeof ai->_ai_inet.sin_addr);
	memcpy(&ai->_ai_inet.sin_addr, he->h_addr_list[0], he->h_length);

	if (serv != NULL) {
		unsigned long port;
		char *end;

		port = strtoul(serv, &end, 0);
		if (*end == '\0') {
			ai->_ai_inet.sin_port = htons(port);
		} else {
			struct protoent *pe;
			struct servent *se;

			pe = getprotobynumber(ai->ai_protocol);
			if (pe == NULL) {
				free(ai);
				return (2);
			}

			se = getservbyname(serv, pe->p_name);
			if (se == NULL) {
				free(ai);
				return (3);
			}

			ai->_ai_inet.sin_port = se->s_port;
		}
	}

	*aip = ai;
	return (0);
}

static const char *
gai_strerror(int rv)
{
	switch (rv) {
	case 1:
		return "could not look up host";
	case 2:
		return "could not look up protocol";
	case 3:
		return "could not look up service";
	default:
		NOTREACHED(log_);
		return "internal error";
	}
}

static void
freeaddrinfo(struct addrinfo *ai)
{
	delete ai;
}

#define	NI_MAXHOST	1024
#define	NI_MAXSERV	16
#define	NI_NUMERICHOST	0
#define	NI_NUMERICSERV	0

static int
getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
	    size_t hostlen, char *serv, size_t servlen, int flags)
{
	struct sockaddr_in *inet = (struct sockaddr_in *)sa;
	char *p;

	if (inet->sin_family != AF_INET || salen != sizeof *inet || flags != 0)
		return (ENOTSUP);

	p = inet_ntoa(inet->sin_addr);
	if (p == NULL)
		return (EINVAL);

	snprintf(host, hostlen, "%s", p);
	snprintf(serv, servlen, "%hu", ntohs(inet->sin_port));

	return (0);
}
#endif /* __OPENNT */


bool
socket_address::operator() (int domain, int socktype, int protocol, const std::string& str)
{
	switch (domain) {
#if defined(AF_INET6)
	case AF_UNSPEC:
	case AF_INET6:
#endif
	case AF_INET:
	{
		std::string name;
		std::string service;

		std::string::size_type pos = str.find(']');
		if (pos != std::string::npos) {
			if (pos < 3 || str[0] != '[' || str[pos + 1] != ':')
				return (false);
			name = str.substr(1, pos - 1);
			service = str.substr(pos + 2);
		} else {
			pos = str.find(':');
			if (pos == std::string::npos)
				return (false);
			name = str.substr(0, pos);
			service = str.substr(pos + 1);
		}

		struct addrinfo hints;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = domain;
		hints.ai_socktype = socktype;
		hints.ai_protocol = protocol;

		/*
		 * Mac OS X ~Snow Leopard~ cannot handle a service name
		 * of "0" where older Mac OS X could.  Don't know if
		 * that is because it's not in services(5) or due to
		 * some attempt at input validation.  So work around it
		 * here, sigh.
		 */
		const char *servptr;
		if (service == "" || service == "0")
			servptr = NULL;
		else
			servptr = service.c_str();

		struct addrinfo *ai;
		int rv = getaddrinfo(name.c_str(), servptr, &hints, &ai);
		if (rv != 0) {
			ERROR("/socket/address") << "Could not look up " << str << ": " << gai_strerror(rv);
			return (false);
		}

		/*
		 * Just use the first one.
		 * XXX Will we ever get one in the wrong family?  Is the hint mandatory?
		 */
		memcpy(&addr_.sockaddr_, ai->ai_addr, ai->ai_addrlen);
		addrlen_ = ai->ai_addrlen;

		freeaddrinfo(ai);
		break;
	}
	case AF_UNIX:
		addr_.unix_.sun_family = domain;
		snprintf(addr_.unix_.sun_path, sizeof addr_.unix_.sun_path, "%s", str.c_str());
		addrlen_ = sizeof addr_.unix_;
#if !defined(__linux__) && !defined(__sun__) && !defined(__OPENNT)
		addr_.unix_.sun_len = addrlen_;
#endif
		break;
	default:
		ERROR("/socket/address") << "Addresss family not supported: " << domain;
		return (false);
	}

	return (true);
}


socket_address::operator std::string (void) const
{
	std::ostringstream str;

	switch (addr_.sockaddr_.sa_family) {
#if defined(AF_INET6)
	case AF_INET6:
#endif
	case AF_INET:
	{
		char host[NI_MAXHOST], serv[NI_MAXSERV];
		int flags;
		int rv;

		/* XXX If we ever support !NI_NUMERICSERV, need NI_DGRAM for UDP.  */
		flags = NI_NUMERICHOST | NI_NUMERICSERV;

		rv = getnameinfo(&addr_.sockaddr_, addrlen_, host, sizeof host, serv, sizeof serv, flags);
		if (rv == -1)
			return ("<getnameinfo-error>");

		str << '[' << host << ']' << ':' << serv;
		break;
	}
	case AF_UNIX:
		str << addr_.unix_.sun_path;
		break;
	default:
		return  ("<address-family-not-supported>");
	}

	return (str.str());
}



