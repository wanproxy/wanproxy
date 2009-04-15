#include <sys/errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/serial.h>

SerialPort::SerialPort(int fd)
: FileDescriptor(fd),
  log_("/serial/device")
{
}

SerialPort::~SerialPort()
{
}

SerialPort *
SerialPort::open(const std::string& path, const Speed& speed)
{
	struct termios control;
	int error;
	int fd;

	fd = ::open(path.c_str(), O_RDWR | O_NOCTTY);
	if (fd == -1)
		return (NULL);

	error = ::ioctl(fd, TIOCEXCL);
	if (error != 0) {
		ERROR("/serial") << "ioctl(TIOEXCL): " << errno;
		::close(fd);
		return (NULL);
	}

	error = tcgetattr(fd, &control);
	if (error != 0) {
		ERROR("/serial") << "tcgetattr failed: " << errno;
		::close(fd);
		return (NULL);
	}

	cfmakeraw(&control);

	switch (speed()) {
	case 115200:
		error = cfsetspeed(&control, B115200);
		if (error != 0) {
			ERROR("/serial") << "cfsetspeed failed: " << errno;
			::close(fd);
			return (NULL);
		}
		break;
	default:
		NOTREACHED();
	}

	control.c_cflag &= ~PARENB;
	control.c_cflag &= ~CSTOPB;
	control.c_cflag &= ~CSIZE;
	control.c_cflag |= CS8;
	control.c_cflag |= CLOCAL;

	control.c_iflag |= IXON | IXOFF;

	control.c_cc[VMIN] = 0;
	control.c_cc[VTIME] = 0;

	error = tcsetattr(fd, TCSAFLUSH, &control);
	if (error != 0) {
		ERROR("/serial") << "tcsetattr failed: " << errno;
		::close(fd);
		return (NULL);
	}

	return (new SerialPort(fd));
}
