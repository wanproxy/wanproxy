#include <sys/errno.h>
#include <sys/uio.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <common/buffer.h>

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
			close_callback_->event(Event(Event::Error, errno));
			Action *a = EventSystem::instance()->schedule(close_callback_);
			close_action_ = a;
			close_callback_ = NULL;
			break;
		}
		return;
	}
	fd_ = -1;
	close_callback_->event(Event(Event::Done, 0));
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

	/* XXX Read into an iovec of BufferSegments?  */
	uint8_t data[IO_READ_BUFFER_SIZE];
	ssize_t len = ::read(fd_, data, sizeof data);
	if (len == -1) {
		switch (errno) {
		case EAGAIN:
			read_action_ = read_schedule();
			break;
		default:
			read_callback_->event(Event(Event::Error, errno, read_buffer_));
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
	 * If we get an EOS from EventPoll, should we just terminate after that
	 * one read?  How can we tell how much data is still available to be
	 * read?  Will we get it all in one read?  Eventually, one would expect
	 * to get a read with a length of 0, or an error, but will kevent even
	 * continue to fire off read events?
	 */
	if (len == 0) {
		read_callback_->event(Event(Event::EOS, 0, read_buffer_));
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
		read_callback_->event(Event(Event::Done, 0, Buffer(read_buffer_, read_amount_)));
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
	case Event::EOS:
	case Event::Done:
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	/* XXX This doesn't handle UDP nicely.  Right?  */
	struct iovec iov[IOV_MAX];
	size_t iovcnt = write_buffer_.fill_iovec(iov, IOV_MAX);

	ssize_t len = ::writev(fd_, iov, iovcnt);
	if (len == -1) {
		switch (errno) {
		case EAGAIN:
			write_action_ = write_schedule();
			break;
		default:
			write_callback_->event(Event(Event::Error, errno));
			Action *a = EventSystem::instance()->schedule(write_callback_);
			write_action_ = a;
			write_callback_ = NULL;
			break;
		}
		return;
	}

	write_buffer_.skip(len);

	if (write_buffer_.empty()) {
		write_callback_->event(Event(Event::Done, 0));
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
{ }

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
