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

struct socket_address {
	union address_union {
		struct sockaddr sockaddr_;
		struct sockaddr_in inet_;
		struct sockaddr_in6 inet6_;
		struct sockaddr_un unix_;

		operator std::string (void) const
		{
			/* XXX getnameinfo(3); */
			char address[256]; /* XXX*/
			const char *p;

			std::ostringstream str;

			switch (sockaddr_.sa_family) {
			case AF_INET:
				p = ::inet_ntop(inet_.sin_family,
						&inet_.sin_addr,
						address, sizeof address);
				ASSERT(p != NULL);

				str << '[' << address << ']' << ':' << ntohs(inet_.sin_port);
				break;
			case AF_INET6:
				p = ::inet_ntop(inet6_.sin6_family,
						&inet6_.sin6_addr,
						address, sizeof address);
				ASSERT(p != NULL);

				str << '[' << address << ']' << ':' << ntohs(inet6_.sin6_port);
				break;
			default:
				return ("<unsupported-address-family>");
			}

			return (str.str());
		}
	} addr_;
	size_t addrlen_;

	socket_address(void)
	: addr_(),
	  addrlen_(0)
	{
		memset(&addr_, 0, sizeof addr_);
	}

	bool operator() (int domain, int socktype, int protocol, const std::string& str)
	{
		switch (domain) {
		case AF_UNSPEC:
		case AF_INET6:
		case AF_INET: {
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

			struct addrinfo *ai;
			int rv = getaddrinfo(name.c_str(), service.c_str(), &hints, &ai);
			if (rv != 0) {
				ERROR("/socket/address") << "Could not look up " << str << ": " << strerror(errno);
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
#if !defined(__linux__) && !defined(__sun__)
			addr_.unix_.sun_len = addrlen_;
#endif
			break;
		default:
			ERROR("/socket/address") << "Addresss family not supported: " << domain;
			return (false);
		}

		return (true);
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

	int rv = ::bind(fd_, &addr.addr_.sockaddr_, addr.addrlen_);
	if (rv == -1)
		return (false);

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

std::string
Socket::getpeername(void) const
{
	socket_address::address_union un;
	socklen_t len;
	int rv;

	len = sizeof un;
	rv = ::getpeername(fd_, &un.sockaddr_, &len);
	if (rv == -1)
		return ("<unknown>");

	/* XXX Check len.  */

	return ((std::string)un);
}

std::string
Socket::getsockname(void) const
{
	socket_address::address_union un;
	socklen_t len;
	int rv;

	len = sizeof un;
	rv = ::getsockname(fd_, &un.sockaddr_, &len);
	if (rv == -1)
		return ("<unknown>");

	/* XXX Check len.  */

	return ((std::string)un);
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

	case SocketAddressFamilyIPv4:
		domainnum = AF_INET;
		break;

	case SocketAddressFamilyIPv6:
		domainnum = AF_INET6;
		break;
	
	case SocketAddressFamilyUnix:
		domainnum = AF_UNIX;
		break;
	
	default:
		ERROR("/socket") << "Unsupported address family.";
		return (NULL);
	}

	int s = ::socket(domainnum, typenum, protonum);
	if (s == -1) {
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
		ERROR("/socket") << "Could not create socket: " << strerror(errno);
		return (NULL);
	}

	return (new Socket(s, domainnum, typenum, protonum));
}
