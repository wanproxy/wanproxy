#include <fcntl.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file.h>

File::File(int fd)
: FileDescriptor(fd)
{
}

File::~File()
{
}

FileDescriptor *
File::open(const std::string& path, bool read, bool write)
{
	int flags;
	int fd;

	if (read && !write)
		flags = O_RDONLY;
	else if (write && !read)
		flags = O_WRONLY;
	else if (write && read)
		flags = O_RDWR;
	else
		HALT("/file") << "Must be opening file for read or write.";

	fd = ::open(path.c_str(), flags);
	if (fd == -1)
		return (NULL);

	return (new File(fd));
}
