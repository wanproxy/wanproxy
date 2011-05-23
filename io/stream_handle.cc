#include <fcntl.h>

#include <event/event_callback.h>

#include <io/stream_handle.h>
#include <io/io_system.h>

StreamHandle::StreamHandle(int fd)
: log_("/file/descriptor"),
  fd_(fd)
{
	int flags = ::fcntl(fd_, F_GETFL, 0);
	if (flags == -1) {
		ERROR(log_) << "Could not get flags for file descriptor.";
	} else {
		flags |= O_NONBLOCK;

		flags = ::fcntl(fd_, F_SETFL, flags);
		if (flags == -1)
			ERROR(log_) << "Could not set flags for file descriptor, some operations may block.";
	}

	IOSystem::instance()->attach(fd_, this);
}

StreamHandle::~StreamHandle()
{
	IOSystem::instance()->detach(fd_, this);
}

Action *
StreamHandle::close(SimpleCallback *cb)
{
	return (IOSystem::instance()->close(fd_, this, cb));
}

Action *
StreamHandle::read(size_t amount, EventCallback *cb)
{
	return (IOSystem::instance()->read(fd_, this, -1, amount, cb));
}

Action *
StreamHandle::write(Buffer *buffer, EventCallback *cb)
{
	return (IOSystem::instance()->write(fd_, this, -1, buffer, cb));
}

Action *
StreamHandle::shutdown(bool, bool, EventCallback *cb)
{
	cb->param(Event::Error);
	return (cb->schedule());
}
