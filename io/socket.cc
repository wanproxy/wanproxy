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

#include <io/file_descriptor.h>
#include <io/socket.h>

/*
 * XXX
 *
 * socket_name() and get*name() do not work with AF_UNIX, but the AF_UNIX stuff
 * is pretty temporary -- soon all of the bind()/connect() variants will be
 * collapsed in to one, I hope.
 */

static bool socket_address(struct sockaddr_in *, int, const std::string&, uint16_t);
static std::string socket_name(struct sockaddr_in *);

Socket::Socket(int fd, int domain)
: FileDescriptor(fd),
  log_("/socket"),
  domain_(domain),
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
	struct sockaddr_un bind_address;

	/* XXX Check for length.  */

	memset(&bind_address, 0, sizeof bind_address);
#if !defined(__linux__) && !defined(__sun__)
	bind_address.sun_len = sizeof bind_address;
#endif
	bind_address.sun_family = domain_;
	strncpy(bind_address.sun_path, name.c_str(), sizeof bind_address.sun_path);

	int rv = ::bind(fd_, (struct sockaddr *)&bind_address,
			sizeof bind_address);
	if (rv == -1)
		return (false);

	return (true);
}

bool
Socket::bind(const std::string& name, unsigned *portp)
{
	struct sockaddr_in bind_address;

	if (!socket_address(&bind_address, domain_, name, *portp))
		return (false);

	int rv = ::bind(fd_, (struct sockaddr *)&bind_address,
			sizeof bind_address);
	if (rv == -1)
		return (false);

	if (*portp == 0) {
		socklen_t addrlen = sizeof bind_address;
		rv = ::getsockname(fd_, (struct sockaddr *)&bind_address,
				   &addrlen);
		if (rv == -1 || addrlen != sizeof bind_address)
			return (false);
		ASSERT(bind_address.sin_family == domain_);
		*portp = ntohs(bind_address.sin_port);
	}

	return (true);
}

Action *
Socket::connect(const std::string& name, EventCallback *cb)
{
	ASSERT(connect_callback_ == NULL);
	ASSERT(connect_action_ == NULL);

	struct sockaddr_un connect_address;

	/* XXX Check for length.  */

	memset(&connect_address, 0, sizeof connect_address);
#if !defined(__linux__) && !defined(__sun__)
	connect_address.sun_len = sizeof connect_address;
#endif
	connect_address.sun_family = domain_;
	strncpy(connect_address.sun_path, name.c_str(), sizeof connect_address.sun_path);

	return (this->connect((struct sockaddr *)&connect_address,
			      sizeof connect_address, cb));
}

Action *
Socket::connect(const std::string& name, unsigned port, EventCallback *cb)
{
	ASSERT(connect_callback_ == NULL);
	ASSERT(connect_action_ == NULL);

	struct sockaddr_in connect_address;

	if (!socket_address(&connect_address, domain_, name, port)) {
		cb->event(Event(Event::Error, ENOENT));
		Action *a = EventSystem::instance()->schedule(cb);
		return (a);
	}

	return (this->connect((struct sockaddr *)&connect_address,
			      sizeof connect_address, cb));
}

Action *
Socket::connect(uint32_t ip, uint16_t port, EventCallback *cb)
{
	ASSERT(connect_callback_ == NULL);
	ASSERT(connect_action_ == NULL);

	struct sockaddr_in connect_address;

	memset(&connect_address, 0, sizeof connect_address);
#if !defined(__linux__) && !defined(__sun__)
	connect_address.sin_len = sizeof connect_address;
#endif
	connect_address.sin_family = domain_;
	connect_address.sin_addr.s_addr = BigEndian::encode(ip);
	connect_address.sin_port = BigEndian::encode(port);

	return (this->connect((struct sockaddr *)&connect_address,
			      sizeof connect_address, cb));
}

Action *
Socket::connect(struct sockaddr *sap, size_t salen, EventCallback *cb)
{
	ASSERT(connect_callback_ == NULL);
	ASSERT(connect_action_ == NULL);

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

	int rv = ::connect(fd_, sap, salen);
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
		HALT(log_) << "Connect returned " << errno;
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
	struct sockaddr_in addr;
	socklen_t len;
	int rv;

	len = sizeof addr;
	rv = ::getpeername(fd_, (struct sockaddr *)&addr, &len);
	if (rv == -1)
		return ("<unknown>");

	if (len != sizeof addr)
		return ("<wrong-domain>");

	return (socket_name(&addr));
}

std::string
Socket::getsockname(void) const
{
	struct sockaddr_in addr;
	socklen_t len;
	int rv;

	len = sizeof addr;
	rv = ::getsockname(fd_, (struct sockaddr *)&addr, &len);
	if (rv == -1)
		return ("<unknown>");

	if (len != sizeof addr)
		return ("<wrong-domain>");

	return (socket_name(&addr));
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

	Socket *child = new Socket(s, domain_);
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
Socket::create(int domain, int type, const std::string& protocol)
{
	int protonum;

	if (protocol == "") {
		protonum = 0;
	} else {
		struct protoent *proto = getprotobyname(protocol.c_str());
		if (proto == NULL)
			HALT("/socket") << "Invalid protocol: " << protocol;
		protonum = proto->p_proto;
	}

	int s = ::socket(domain, type, protonum);
	if (s == -1)
		return (NULL);

	return (new Socket(s, domain));
}

static bool
socket_address(struct sockaddr_in *sinp, int domain, const std::string& name,
	       uint16_t port)
{
	struct hostent *host;
#if !defined(__sun__)
	host = gethostbyname2(name.c_str(), domain);
#else
	host = gethostbyname(name.c_str()); /* Hope for the best!  */
#endif
	if (host == NULL) {
		return (false);
	}
	/*
	 * XXX
	 * Remove this assertion and find the resolution which yields the right
	 * domain and fail if we can't find one.  Blowing up here is just such
	 * a bad idea.
	 */
	ASSERT(host->h_addrtype == domain);
	ASSERT(host->h_length == sizeof sinp->sin_addr);

	memset(sinp, 0, sizeof *sinp);
#if !defined(__linux__) && !defined(__sun__)
	sinp->sin_len = sizeof *sinp;
#endif
	sinp->sin_family = domain;
	memcpy(&sinp->sin_addr, host->h_addr_list[0], sizeof sinp->sin_addr);
	sinp->sin_port = htons(port);
	return (true);
}

static std::string
socket_name(struct sockaddr_in *sinp)
{
	/* XXX getnameinfo(3) */
	char address[256]; /* XXX */
	const char *p;

	p = ::inet_ntop(sinp->sin_family, &sinp->sin_addr, address,
			sizeof address);
	ASSERT(p != NULL);

	std::ostringstream os;
	os << address << ':' << ntohs(sinp->sin_port);
	return (os.str());
}
