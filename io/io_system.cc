#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include <common/buffer.h>
#include <common/limits.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/io_system.h>

#define	IO_READ_BUFFER_SIZE	65536

IOSystem::Handle::Handle(int fd, Channel *owner)
: log_("/io/system/handle"),
  fd_(fd),
  owner_(owner),
  close_callback_(NULL),
  close_action_(NULL),
  read_amount_(0),
  read_buffer_(),
  read_callback_(NULL),
  read_action_(NULL),
  write_buffer_(),
  write_callback_(NULL),
  write_action_(NULL)
{ }

IOSystem::Handle::~Handle()
{
	ASSERT(fd_ == -1);

	ASSERT(close_action_ == NULL);
	ASSERT(close_callback_ == NULL);

	ASSERT(read_action_ == NULL);
	ASSERT(read_callback_ == NULL);

	ASSERT(write_action_ == NULL);
	ASSERT(write_callback_ == NULL);
}

void
IOSystem::Handle::close_callback(void)
{
	close_action_->cancel();
	close_action_ = NULL;

	ASSERT(fd_ != -1);
	int rv = ::close(fd_);
	if (rv == -1) {
		switch (errno) {
		case EAGAIN:
			close_action_ = close_schedule();
			break;
		default:
			close_callback_->param(Event(Event::Error, errno));
			Action *a = EventSystem::instance()->schedule(close_callback_);
			close_action_ = a;
			close_callback_ = NULL;
			break;
		}
		return;
	}
	fd_ = -1;
	close_callback_->param(Event::Done);
	Action *a = EventSystem::instance()->schedule(close_callback_);
	close_action_ = a;
	close_callback_ = NULL;
}

void
IOSystem::Handle::close_cancel(void)
{
	ASSERT(close_action_ != NULL);
	close_action_->cancel();
	close_action_ = NULL;

	if (close_callback_ != NULL) {
		delete close_callback_;
		close_callback_ = NULL;
	}
}

Action *
IOSystem::Handle::close_schedule(void)
{
	ASSERT(close_action_ == NULL);
	Callback *cb = callback(this, &IOSystem::Handle::close_callback);
	Action *a = EventSystem::instance()->schedule(cb);
	return (a);
}

void
IOSystem::Handle::read_callback(Event e)
{
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::EOS:
	case Event::Done:
		break;
	case Event::Error: {
		DEBUG(log_) << "Poll returned error: " << e;
		read_callback_->param(e);
		Action *a = EventSystem::instance()->schedule(read_callback_);
		read_action_ = a;
		read_callback_ = NULL;
		return;
	}
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	size_t rlen;

	if (read_amount_ == 0)
		rlen = IO_READ_BUFFER_SIZE;
	else {
		rlen = read_amount_ - read_buffer_.length();
		ASSERT(rlen != 0);
		if (rlen > IO_READ_BUFFER_SIZE)
			rlen = IO_READ_BUFFER_SIZE;
	}

	/*
	 * A bit of discussion is warranted on this:
	 *
	 * In tack, IOV_MAX BufferSegments are allocated and read in to with
	 * readv(2), and then the lengths are adjusted and the ones that are
	 * empty are freed.  It's also possible to set the expected lengths
	 * first (and only allocate
	 * 	roundup(rlen, BUFFER_SEGMENT_SIZE) / BUFFER_SEGMENT_SIZE
	 * BufferSegments, though really IOV_MAX (or some chosen number) seems
	 * a bit better since most of our reads right now are read_amount_==0)
	 * and put them into a Buffer and trim the leftovers, which is a bit
	 * nicer.
	 *
	 * Since our read_amount_ is usually 0, though, we're kind of at the
	 * mercy chance (well, not really) as to how much data we will read,
	 * which means a sizable amount of thrashing of memory; allocating and
	 * freeing BufferSegments.
	 *
	 * By comparison, stack space is cheap in userland and allocating 64K
	 * of it here is pretty painless.  Reading to it is fast and then
	 * copying only what we need into BufferSegments isn't very costly.
	 * Indeed, since readv can't sparsely-populate each data pointer, it
	 * has to do some data shuffling, already.  Benchmarking would be a
	 * good idea, but it seems like there are arguments both ways.  Of
	 * course, tack is very fast and this code path hasn't been thrashed
	 * half so much.  When tack is adjusted to use the IO system and Pipes
	 * in the future, if performance degradation is noticeable, perhaps
	 * it will worth it to switch.  For now, absence any idea of the real
	 * read sizes in the wild, doing nothing and not thrashing memory is
	 * decidedly more appealing.
	 */
	uint8_t data[IO_READ_BUFFER_SIZE];
	ssize_t len = ::read(fd_, data, sizeof data);
	if (len == -1) {
		switch (errno) {
		case EAGAIN:
			read_action_ = read_schedule();
			break;
		default:
			read_callback_->param(Event(Event::Error, errno, read_buffer_));
			Action *a = EventSystem::instance()->schedule(read_callback_);
			read_action_ = a;
			read_callback_ = NULL;
			read_buffer_.clear();
			read_amount_ = 0;
			break;
		}
		return;
	}

	/*
	 * XXX
	 * If we get a short read from readv and detected EOS from EventPoll is
	 * that good enough, instead?  We can keep reading until we get a 0, sure,
	 * but if things other than network conditions influence whether reads
	 * would block (and whether non-blocking reads return), there could be
	 * more data waiting, and so we shouldn't just use a short read as an
	 * indicator?
	 */
	if (len == 0) {
		read_callback_->param(Event(Event::EOS, read_buffer_));
		Action *a = EventSystem::instance()->schedule(read_callback_);
		read_action_ = a;
		read_callback_ = NULL;
		read_buffer_.clear();
		read_amount_ = 0;
		return;
	}

	read_buffer_.append(data, len);
	read_action_ = read_schedule();
}

void
IOSystem::Handle::read_cancel(void)
{
	ASSERT(read_action_ != NULL);
	read_action_->cancel();
	read_action_ = NULL;

	if (read_callback_ != NULL) {
		delete read_callback_;
		read_callback_ = NULL;
	}
}

Action *
IOSystem::Handle::read_schedule(void)
{
	ASSERT(read_action_ == NULL);

	if (!read_buffer_.empty() && read_buffer_.length() >= read_amount_) {
		if (read_amount_ == 0)
			read_amount_ = read_buffer_.length();
		read_callback_->param(Event(Event::Done, Buffer(read_buffer_, read_amount_)));
		Action *a = EventSystem::instance()->schedule(read_callback_);
		read_callback_ = NULL;
		read_buffer_.skip(read_amount_);
		read_amount_ = 0;
		return (a);
	}

	EventCallback *cb = callback(this, &IOSystem::Handle::read_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, fd_, cb);
	return (a);
}

void
IOSystem::Handle::write_callback(Event e)
{
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error: {
		DEBUG(log_) << "Poll returned error: " << e;
		write_callback_->param(e);
		Action *a = EventSystem::instance()->schedule(write_callback_);
		write_action_ = a;
		write_callback_ = NULL;
		return;
	}
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	/* XXX This doesn't handle UDP nicely.  Right?  */
	/* XXX If a UDP packet is > IOV_MAX segments, this will break it.  */
	struct iovec iov[IOV_MAX];
	size_t iovcnt = write_buffer_.fill_iovec(iov, IOV_MAX);

	ssize_t len = ::writev(fd_, iov, iovcnt);
	if (len == -1) {
		switch (errno) {
		case EAGAIN:
			write_action_ = write_schedule();
			break;
		default:
			write_callback_->param(Event(Event::Error, errno));
			Action *a = EventSystem::instance()->schedule(write_callback_);
			write_action_ = a;
			write_callback_ = NULL;
			break;
		}
		return;
	}

	write_buffer_.skip(len);

	if (write_buffer_.empty()) {
		write_callback_->param(Event::Done);
		Action *a = EventSystem::instance()->schedule(write_callback_);
		write_action_ = a;
		write_callback_ = NULL;
		return;
	}
	write_action_ = write_schedule();
}

void
IOSystem::Handle::write_cancel(void)
{
	ASSERT(write_action_ != NULL);
	write_action_->cancel();
	write_action_ = NULL;

	if (write_callback_ != NULL) {
		delete write_callback_;
		write_callback_ = NULL;
	}
}

Action *
IOSystem::Handle::write_schedule(void)
{
	ASSERT(write_action_ == NULL);
	EventCallback *cb = callback(this, &IOSystem::Handle::write_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Writable, fd_, cb);
	return (a);
}

IOSystem::IOSystem(void)
: log_("/io/system"),
  handle_map_()
{
	/*
	 * Prepare system to handle IO.
	 */
	INFO(log_) << "Starting IO system.";

	/*
	 * Disable SIGPIPE.
	 *
	 * Because errors are returned asynchronously and may occur at any time,
	 * there may be a pending write to a file descriptor which has
	 * previously thrown an error.  There are changes that could be made to
	 * the scheduler to work around this, but they are not desirable.
	 */
	if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		HALT(log_) << "Could not disable SIGPIPE.";

	/*
	 * Up the file descriptor limit.
	 *
	 * Probably this should be configurable, but there's no harm on modern
	 * systems and for the performance-critical applications using the IO
	 * system, more file descriptors is better.
	 */
	struct rlimit rlim;
	int rv = ::getrlimit(RLIMIT_NOFILE, &rlim);
	if (rv == 0) {
		if (rlim.rlim_cur < rlim.rlim_max) {
			rlim.rlim_cur = rlim.rlim_max;

			rv = ::setrlimit(RLIMIT_NOFILE, &rlim);
			if (rv == -1) {
				INFO(log_) << "Unable to increase file descriptor limit.";
			}
		}
	} else {
		INFO(log_) << "Unable to get file descriptor limit.";
	}
}

IOSystem::~IOSystem()
{
	ASSERT(handle_map_.empty());
}

void
IOSystem::attach(int fd, Channel *owner)
{
	ASSERT(handle_map_.find(handle_key_t(fd, owner)) == handle_map_.end());
	handle_map_[handle_key_t(fd, owner)] = new IOSystem::Handle(fd, owner);
}

void
IOSystem::detach(int fd, Channel *owner)
{
	handle_map_t::iterator it;
	IOSystem::Handle *h;

	it = handle_map_.find(handle_key_t(fd, owner));
	ASSERT(it != handle_map_.end());

	h = it->second;
	ASSERT(h != NULL);
	ASSERT(h->owner_ == owner);

	handle_map_.erase(it);
	delete h;
}

Action *
IOSystem::close(int fd, Channel *owner, EventCallback *cb)
{
	IOSystem::Handle *h;

	h = handle_map_[handle_key_t(fd, owner)];
	ASSERT(h != NULL);

	ASSERT(h->close_callback_ == NULL);
	ASSERT(h->close_action_ == NULL);

	ASSERT(h->read_callback_ == NULL);
	ASSERT(h->read_action_ == NULL);

	ASSERT(h->write_callback_ == NULL);
	ASSERT(h->write_action_ == NULL);

	ASSERT(h->fd_ != -1);

	h->close_callback_ = cb;
	h->close_action_ = h->close_schedule();
	return (cancellation(h, &IOSystem::Handle::close_cancel));
}

Action *
IOSystem::read(int fd, Channel *owner, size_t amount, EventCallback *cb)
{
	IOSystem::Handle *h;

	h = handle_map_[handle_key_t(fd, owner)];
	ASSERT(h != NULL);

	ASSERT(h->read_callback_ == NULL);
	ASSERT(h->read_action_ == NULL);

	h->read_amount_ = amount;
	h->read_callback_ = cb;
	h->read_action_ = h->read_schedule();
	return (cancellation(h, &IOSystem::Handle::read_cancel));
}

Action *
IOSystem::write(int fd, Channel *owner, Buffer *buffer, EventCallback *cb)
{
	IOSystem::Handle *h;

	h = handle_map_[handle_key_t(fd, owner)];
	ASSERT(h != NULL);

	ASSERT(h->write_callback_ == NULL);
	ASSERT(h->write_action_ == NULL);
	ASSERT(h->write_buffer_.empty());

	ASSERT(!buffer->empty());
	h->write_buffer_.append(buffer);
	buffer->clear();

	h->write_callback_ = cb;
	h->write_action_ = h->write_schedule();
	return (cancellation(h, &IOSystem::Handle::write_cancel));
}
