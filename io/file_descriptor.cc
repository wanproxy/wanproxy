#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <limits.h>
#include <termios.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>

FileDescriptor::FileDescriptor(int fd)
: log_("/file/descriptor"),
  fd_(fd),
  close_callback_(NULL),
  close_action_(NULL),
  read_amount_(0),
  read_buffer_(),
  read_callback_(NULL),
  read_action_(NULL),
  write_buffer_(),
  write_callback_(NULL),
  write_action_(NULL)
{
	int flags = ::fcntl(fd_, F_GETFL, 0);
	if (flags == -1)
		HALT(log_) << "Could not get flags for file descriptor.";
	flags |= O_NONBLOCK;
	flags = ::fcntl(fd_, F_SETFL, flags);
	if (flags == -1)
		HALT(log_) << "Could not set flags for file descriptor.";
}

FileDescriptor::~FileDescriptor()
{
	ASSERT(fd_ == -1);

	ASSERT(close_action_ == NULL);
	ASSERT(close_callback_ == NULL);

	ASSERT(read_action_ == NULL);
	ASSERT(read_callback_ == NULL);

	ASSERT(write_action_ == NULL);
	ASSERT(write_callback_ == NULL);
}

Action *
FileDescriptor::close(EventCallback *cb)
{
	ASSERT(close_callback_ == NULL);
	ASSERT(close_action_ == NULL);

	ASSERT(read_callback_ == NULL);
	ASSERT(read_action_ == NULL);

	ASSERT(write_callback_ == NULL);
	ASSERT(write_action_ == NULL);

	ASSERT(fd_ != -1);

	close_callback_ = cb;
	close_action_ = close_schedule();
	return (cancellation(this, &FileDescriptor::close_cancel));
}

Action *
FileDescriptor::read(size_t amount, EventCallback *cb)
{
	ASSERT(read_callback_ == NULL);
	ASSERT(read_action_ == NULL);

	read_amount_ = amount;
	read_callback_ = cb;
	read_action_ = read_schedule();
	return (cancellation(this, &FileDescriptor::read_cancel));
}

Action *
FileDescriptor::write(Buffer *buffer, EventCallback *cb)
{
	ASSERT(write_callback_ == NULL);
	ASSERT(write_action_ == NULL);
	ASSERT(write_buffer_.empty());

	write_buffer_.append(buffer);
	buffer->clear();

	write_callback_ = cb;
	write_action_ = write_schedule();
	return (cancellation(this, &FileDescriptor::write_cancel));
}

void
FileDescriptor::close_callback(void)
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
FileDescriptor::close_cancel(void)
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
FileDescriptor::close_schedule(void)
{
	ASSERT(close_action_ == NULL);
	Callback *cb = callback(this, &FileDescriptor::close_callback);
	Action *a = EventSystem::instance()->schedule(cb);
	return (a);
}

void
FileDescriptor::read_callback(Event e, void *)
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
		rlen = 65536;
	else
		rlen = read_amount_ - read_buffer_.length();
	ASSERT(rlen != 0);

	uint8_t data[rlen];
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
FileDescriptor::read_cancel(void)
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
FileDescriptor::read_schedule(void)
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

	EventCallback *cb = callback(this, &FileDescriptor::read_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, fd_, cb);
	return (a);
}

void
FileDescriptor::write_callback(Event e, void *)
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
FileDescriptor::write_cancel(void)
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
FileDescriptor::write_schedule(void)
{
	ASSERT(write_action_ == NULL);
	EventCallback *cb = callback(this, &FileDescriptor::write_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Writable, fd_, cb);
	return (a);
}
