#include <fcntl.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/io_system.h>

FileDescriptor::FileDescriptor(int fd)
: log_("/file/descriptor"),
  fd_(fd)
{
	int flags = ::fcntl(fd_, F_GETFL, 0);

	if (flags == -1)
		HALT(log_) << "Could not get flags for file descriptor.";

	flags |= O_NONBLOCK;

	flags = ::fcntl(fd_, F_SETFL, flags);
	if (flags == -1)
		HALT(log_) << "Could not set flags for file descriptor.";

	IOSystem::instance()->attach(fd_, this);
}

FileDescriptor::~FileDescriptor()
{
	IOSystem::instance()->detach(fd_, this);
}

Action *
FileDescriptor::close(EventCallback *cb)
{
	return (IOSystem::instance()->close(fd_, this, cb));
}

Action *
FileDescriptor::read(size_t amount, EventCallback *cb)
{
	return (IOSystem::instance()->read(fd_, this, amount, cb));
}

Action *
FileDescriptor::write(Buffer *buffer, EventCallback *cb)
{
	return (IOSystem::instance()->write(fd_, this, buffer, cb));
}
