#include <sys/errno.h>
#include <sys/select.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_()
{
}

EventPoll::~EventPoll()
{
	ASSERT(read_poll_.empty());
	ASSERT(write_poll_.empty());
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	ASSERT(fd != -1);

	EventPoll::PollHandler *poll_handler;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) == read_poll_.end());
		poll_handler = &read_poll_[fd];
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) == write_poll_.end());
		poll_handler = &write_poll_[fd];
		break;
	default:
		NOTREACHED();
	}
	ASSERT(poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		break;
	}
}

bool
EventPoll::idle(void) const
{
	return (read_poll_.empty() && write_poll_.empty());
}

void
EventPoll::poll(void)
{
	return (wait(0));
}

void
EventPoll::wait(int ms)
{
	poll_handler_map_t::iterator it;
	fd_set read_set, write_set;
	int maxfd;

	/*
	 * XXX
	 * Track minfd, too.
	 */
	maxfd = -1;

	FD_ZERO(&read_set);
	for (it = read_poll_.begin(); it != read_poll_.end(); ++it) {
		FD_SET(it->first, &read_set);
		if (maxfd < it->first)
			maxfd = it->first;
	}

	FD_ZERO(&write_set);
	for (it = write_poll_.begin(); it != write_poll_.end(); ++it) {
		FD_SET(it->first, &write_set);
		if (maxfd < it->first)
			maxfd = it->first;
	}

	/*
	 * XXX Why wait to call idle() until here?
	 */
	if (idle()) {
		if (ms != -1) {
			int rv;

			rv = usleep(ms * 1000);
			ASSERT(rv != -1);
		}
		return;
	}
	ASSERT(maxfd != -1);

	struct timeval tv, *tvp;

	if (ms == -1) {
		tvp = NULL;
	} else {
		tv.tv_sec = ms / 1000;
		tv.tv_usec = ms % 1000;
		tvp = &tv;
	}

	int fdcnt = ::select(maxfd + 1, &read_set, &write_set, NULL, tvp);
	if (fdcnt == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not select.";
	}

	int fd;
	for (fd = 0; fdcnt != 0 && fd <= maxfd; fd++) {
		if (FD_ISSET(fd, &read_set)) {
			ASSERT(read_poll_.find(fd) != read_poll_.end());
			read_poll_[fd].callback(Event(Event::Done, 0));

			ASSERT(fdcnt != 0);
			fdcnt--;
		}

		if (FD_ISSET(fd, &write_set)) {
			ASSERT(write_poll_.find(fd) != write_poll_.end());
			write_poll_[fd].callback(Event(Event::Done, 0));

			ASSERT(fdcnt != 0);
			fdcnt--;
		}
	}
}

void
EventPoll::wait(void)
{
	return (wait(-1));
}
