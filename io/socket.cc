#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>

#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket.h>

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

	ASSERT(he->h_length == sizeof ai->_ai_inet.sin_addr);
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
		NOTREACHED();
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
#endif

struct socket_address {
	union address_union {
		struct sockaddr sockaddr_;
		struct sockaddr_in inet_;
#if defined(AF_INET6)
		struct sockaddr_in6 inet6_;
#endif
		struct sockaddr_un unix_;
	} addr_;
	socklen_t addrlen_;

	socket_address(void)
	: addr_(),
	  addrlen_(0)
	{
		memset(&addr_, 0, sizeof addr_);
	}

	bool operator() (int domain, int socktype, int protocol, const std::string& str)
	{
		switch (domain) {
#if defined(AF_INET6)
		case AF_UNSPEC:
		case AF_INET6:
#endif
		case AF_INET:
		{
			std::string::size_type pos = str.find(']');
			if (pos == std::string::npos)
				return (false);

			if (pos < 3 || str[0] != '[' || str[pos + 1] != ':')
				return (false);

			std::string name(str, 1, pos - 1);
			std::string service(str, pos + 2);

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
			strncpy(addr_.unix_.sun_path, str.c_str(), sizeof addr_.unix_.sun_path);
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

	operator std::string (void) const
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
};

Socket::Socket(int fd, int domain, int socktype, int protocol)
: FileDescriptor(fd),
  log_("/socket"),
  domain_(domain),
  socktype_(socktype),
  protocol_(protocol),
  accept_action_(NULL),
  accept_callback_(NULL),
  connect_callback_(NULL),
  connect_action_(NULL)
{
	ASSERT(fd_ != -1);
}

Socket::~Socket()
{
	ASSERT(accept_action_ == NULL);
	ASSERT(accept_callback_ == NULL);
	ASSERT(connect_callback_ == NULL);
	ASSERT(connect_action_ == NULL);
}

Action *
Socket::accept(EventCallback *cb)
{
	ASSERT(accept_action_ == NULL);
	ASSERT(accept_callback_ == NULL);

	accept_callback_ = cb;
	accept_action_ = accept_schedule();
	return (cancellation(this, &Socket::accept_cancel));
}

bool
Socket::bind(const std::string& name)
{
	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for bind: " << name;
		return (false);
	}

	int on = 1;
	int rv = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
	if (rv == -1) {
		ERROR(log_) << "Could not setsockopt(SO_REUSEADDR): " << strerror(errno);
	}

	rv = ::bind(fd_, &addr.addr_.sockaddr_, addr.addrlen_);
	if (rv == -1) {
		ERROR(log_) << "Could not bind: " << strerror(errno);
		return (false);
	}

	return (true);
}

Action *
Socket::connect(const std::string& name, EventCallback *cb)
{
	ASSERT(connect_callback_ == NULL);
	ASSERT(connect_action_ == NULL);

	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for connect: " << name;
		cb->event(Event(Event::Error, EINVAL));
		return (EventSystem::instance()->schedule(cb));
	}

	/*
	 * TODO
	 *
	 * If the address we are connecting to is the address of another
	 * Socket, set up some sort of zero-copy IO between them.  This is
	 * easy enough if we do it right here.  Trying to do it at the file
	 * descriptor level seems to be really hard since there's a lot of
	 * races possible there.  This way, we're still at the point of the
	 * connection set up.
	 *
	 * We may want to allow the connection to complete so that calls to
	 * getsockname(2) and getpeername(2) do what you'd expect.  We may
	 * still want to poll on the file descriptors even, to make it
	 * possible to use tcpdrop, etc., to reset the connections.  I guess
	 * the thing to do is poll if there's no input ready.
	 */

	int rv = ::connect(fd_, &addr.addr_.sockaddr_, addr.addrlen_);
	switch (rv) {
	case 0:
		cb->event(Event(Event::Done, 0));
		connect_action_ = EventSystem::instance()->schedule(cb);
		break;
	case -1:
		switch (errno) {
		case EINPROGRESS:
			connect_callback_ = cb;
			connect_action_ = connect_schedule();
			break;
		default:
			cb->event(Event(Event::Error, errno));
			connect_action_ = EventSystem::instance()->schedule(cb);
			break;
		}
		break;
	default:
		HALT(log_) << "Connect returned unexpected value: " << rv;
	}
	return (cancellation(this, &Socket::connect_cancel));
}

bool
Socket::listen(int backlog)
{
	int rv = ::listen(fd_, backlog);
	if (rv == -1)
		return (false);
	return (true);
}

bool
Socket::shutdown(bool shut_read, bool shut_write)
{
	int how;

	if (shut_read && shut_write) 
		how = SHUT_RDWR;
	else if (shut_read)
		how = SHUT_RD;
	else if (shut_write)
		how = SHUT_WR;
	else {
		NOTREACHED();
		return (false);
	}

	int rv = ::shutdown(fd_, how);
	if (rv == -1)
		return (false);
	return (true);
}

std::string
Socket::getpeername(void) const
{
	socket_address sa;
	int rv;

	sa.addrlen_ = sizeof sa.addr_;
	rv = ::getpeername(fd_, &sa.addr_.sockaddr_, &sa.addrlen_);
	if (rv == -1)
		return ("<unknown>");

	/* XXX Check len.  */

	return ((std::string)sa);
}

std::string
Socket::getsockname(void) const
{
	socket_address sa;
	int rv;

	sa.addrlen_ = sizeof sa.addr_;
	rv = ::getsockname(fd_, &sa.addr_.sockaddr_, &sa.addrlen_);
	if (rv == -1)
		return ("<unknown>");

	/* XXX Check len.  */

	return ((std::string)sa);
}

void
Socket::accept_callback(Event e)
{
	accept_action_->cancel();
	accept_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::EOS:
	case Event::Error:
		accept_callback_->event(Event(Event::Error, e.error_));
		accept_action_ = EventSystem::instance()->schedule(accept_callback_);
		accept_callback_ = NULL;
		return;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	int s = ::accept(fd_, NULL, NULL);
	if (s == -1) {
		switch (errno) {
		case EAGAIN:
			accept_action_ = accept_schedule();
			return;
		default:
			accept_callback_->event(Event(Event::Error, errno));
			accept_action_ = EventSystem::instance()->schedule(accept_callback_);
			accept_callback_ = NULL;
			return;
		}
	}

	Socket *child = new Socket(s, domain_, socktype_, protocol_);
	accept_callback_->event(Event(Event::Done, 0, (void *)child));
	Action *a = EventSystem::instance()->schedule(accept_callback_);
	accept_action_ = a;
	accept_callback_ = NULL;
}

void
Socket::accept_cancel(void)
{
	ASSERT(accept_action_ != NULL);
	accept_action_->cancel();
	accept_action_ = NULL;

	if (accept_callback_ != NULL) {
		delete accept_callback_;
		accept_callback_ = NULL;
	}
}

Action *
Socket::accept_schedule(void)
{
	EventCallback *cb = callback(this, &Socket::accept_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, fd_, cb);
	return (a);
}

void
Socket::connect_callback(Event e)
{
	connect_action_->cancel();
	connect_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		connect_callback_->event(Event(Event::Done, 0));
		break;
	case Event::EOS:
	case Event::Error:
		connect_callback_->event(Event(Event::Error, e.error_));
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}
	Action *a = EventSystem::instance()->schedule(connect_callback_);
	connect_action_ = a;
	connect_callback_ = NULL;
}

void
Socket::connect_cancel(void)
{
	ASSERT(connect_action_ != NULL);
	connect_action_->cancel();
	connect_action_ = NULL;

	if (connect_callback_ != NULL) {
		delete connect_callback_;
		connect_callback_ = NULL;
	}
}

Action *
Socket::connect_schedule(void)
{
	EventCallback *cb = callback(this, &Socket::connect_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Writable, fd_, cb);
	return (a);
}

Socket *
Socket::create(SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint)
{
	int typenum;

	switch (type) {
	case SocketTypeStream:
		typenum = SOCK_STREAM;
		break;

	case SocketTypeDatagram:
		typenum = SOCK_DGRAM;
		break;

	default:
		ERROR("/socket") << "Unsupported socket type.";
		return (NULL);
	}

	int protonum;

	if (protocol == "") {
		protonum = 0;
	} else {
		struct protoent *proto = getprotobyname(protocol.c_str());
		if (proto == NULL) {
			ERROR("/socket") << "Invalid protocol: " << protocol;
			return (NULL);
		}
		protonum = proto->p_proto;
	}

	int domainnum;

	switch (family) {
	case SocketAddressFamilyIP:
#if defined(AF_INET6)
		if (hint == "") {
			ERROR("/socket") << "Must specify hint address for IP sockets or specify IPv4 or IPv6 explicitly.";
			return (NULL);
		} else {
			socket_address addr;

			if (!addr(AF_UNSPEC, typenum, protonum, hint)) {
				ERROR("/socket") << "Invalid hint: " << hint;
				return (NULL);
			}
			
			/* XXX Just make socket_address::operator() smarter about AF_UNSPEC?  */
			switch (addr.addr_.sockaddr_.sa_family) {
			case AF_INET:
				domainnum = AF_INET;
				break;

			case AF_INET6:
				domainnum = AF_INET6;
				break;

			default:
				ERROR("/socket") << "Unsupported address family for hint: " << hint;
				return (NULL);
			}
			break;
		}
#else
		(void)hint;
		domainnum = AF_INET;
		break;
#endif

	case SocketAddressFamilyIPv4:
		domainnum = AF_INET;
		break;

#if defined(AF_INET6)
	case SocketAddressFamilyIPv6:
		domainnum = AF_INET6;
		break;
#endif
	
	case SocketAddressFamilyUnix:
		domainnum = AF_UNIX;
		break;
	
	default:
		ERROR("/socket") << "Unsupported address family.";
		return (NULL);
	}

	int s = ::socket(domainnum, typenum, protonum);
	if (s == -1) {
#if defined(AF_INET6)
		/*
		 * If we were trying to create an IPv6 socket for a request that
		 * did not specify IPv4 vs. IPv6 and the system claims that the
		 * protocol is not supported, try explicitly creating an IPv4
		 * socket.
		 */
		if (errno == EPROTONOSUPPORT && domainnum == AF_INET6 &&
		    family == SocketAddressFamilyIP) {
			DEBUG("/socket") << "IPv6 socket create failed; trying IPv4.";
			return (Socket::create(SocketAddressFamilyIPv4, type, protocol, hint));
		}
#endif
		ERROR("/socket") << "Could not create socket: " << strerror(errno);
		return (NULL);
	}

	return (new Socket(s, domainnum, typenum, protonum));
}
