#include <fcntl.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>

#include <io/block_handle.h>
#include <io/io_system.h>

BlockHandle::BlockHandle(int fd, size_t bsize)
: BlockChannel(bsize),
  log_("/block/device"),
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

BlockHandle::~BlockHandle()
{
	IOSystem::instance()->detach(fd_, this);
}

Action *
BlockHandle::close(EventCallback *cb)
{
	return (IOSystem::instance()->close(fd_, this, cb));
}

Action *
BlockHandle::read(off_t offset, EventCallback *cb)
{
	return (IOSystem::instance()->read(fd_, this, offset * bsize_, bsize_, cb));
}

Action *
BlockHandle::write(off_t offset, Buffer *buffer, EventCallback *cb)
{
	if (buffer->length() != bsize_) {
		cb->param(Event::Error);
		return (cb->schedule());
	}
	return (IOSystem::instance()->write(fd_, this, offset * bsize_, buffer, cb));
}
