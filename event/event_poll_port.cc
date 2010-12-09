#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <errno.h>
#include <poll.h>
#include <port.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>

/* XXX See kq_map in event_poll_kqueue.c */

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_(),
  port_(port_create())
{
	ASSERT(port_ != -1);
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
	bool dissociate = false;
	int events;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) == read_poll_.end());
		dissociate = write_poll_.find(fd) != write_poll_.end();
		poll_handler = &read_poll_[fd];
		events = POLLIN | (dissociate ? POLLOUT : 0);
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) == write_poll_.end());
		dissociate = read_poll_.find(fd) != read_poll_.end();
		poll_handler = &write_poll_[fd];
		events = POLLOUT | (dissociate ? POLLIN : 0);
		break;
	default:
		NOTREACHED();
	}
	if (dissociate) {
		int rv = ::port_dissociate(port_, PORT_SOURCE_FD, fd);
		if (rv == -1)
			HALT(log_) << "Could not dissociate from port.";
	}
	ASSERT(events != 0);
	int rv = ::port_associate(port_, PORT_SOURCE_FD, fd, events, NULL);
	if (rv == -1)
		HALT(log_) << "Could not associate to port.";
	ASSERT(rv == 0);
	ASSERT(poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	bool associate = false;
	int events = 0;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);

		if (write_poll_.find(fd) != write_poll_.end()) {
			associate = true;
			events = POLLOUT;
		}
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);

		if (read_poll_.find(fd) != read_poll_.end()) {
			associate = true;
			events = POLLIN;
		}
		break;
	}
	int rv = ::port_dissociate(port_, PORT_SOURCE_FD, fd);
	if (rv == -1)
		HALT(log_) << "Could not disassociate from port.";
	ASSERT(rv == 0);
	if (associate) {
		ASSERT(events != 0);
		rv = ::port_associate(port_, PORT_SOURCE_FD, fd, events, NULL);
		if (rv == -1)
			HALT(log_) << "Could not associate to port.";
	}
}

void
EventPoll::wait(int ms)
{
	static const unsigned pevcnt = 128;
	struct timespec ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;

	if (idle()) {
		if (ms != -1) {
			int rv;

			rv = nanosleep(&ts, NULL);
			ASSERT(rv != -1);
		}
		return;
	}

	port_event_t pev[pevcnt];
	unsigned evcnt = 1;
	int rv = port_getn(port_, pev, pevcnt, &evcnt, ms == -1 ? NULL : &ts);
	if (rv == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not poll port.";
	}

	unsigned i;
	for (i = 0; i < evcnt; i++) {
		port_event_t *ev = &pev[i];
		int fd = ev->portev_object;

		/*
		 * The port(3C) mechanism is one-shot, unfortunately, so we
		 * need to re-arm here.  Actually, we're one shot, too, and
		 * so we basically only need to re-arm:
		 * 	events & ~ev->portev_events
		 * but since we don't want to make cancel() processing too
		 * hardcore, we'll settle for this for now.
		 */
		int events = 0;
		if (read_poll_.find(fd) != read_poll_.end())
			events |= POLLIN;
		if (write_poll_.find(fd) != write_poll_.end())
			events |= POLLOUT;
		rv = ::port_associate(port_, PORT_SOURCE_FD, fd, events, NULL);
		if (rv == -1) {
			HALT(log_) << "Could not associate to port.";
		}

		EventPoll::PollHandler *poll_handler;
		if ((ev->portev_events & POLLIN) != 0) {
			ASSERT(read_poll_.find(fd) != read_poll_.end());
			poll_handler = &read_poll_[fd];
			poll_handler->callback(Event::Done);
		}

		if ((ev->portev_events & POLLOUT) != 0) {
			ASSERT(write_poll_.find(fd) != write_poll_.end());
			poll_handler = &write_poll_[fd];
			poll_handler->callback(Event::Done);
		}

		if ((ev->portev_events & POLLIN) == 0 && (ev->portev_events & POLLOUT) == 0) {
			if (read_poll_.find(fd) != read_poll_.end()) {
				poll_handler = &read_poll_[fd];
			} else if (write_poll_.find(fd) != write_poll_.end()) {
				poll_handler = &write_poll_[fd];
			} else {
				HALT(log_) << "Unexpected poll fd.";
				continue;
			}

			if ((ev->portev_events & POLLERR) != 0) {
				poll_handler->callback(Event::Error);
				continue;
			}

			if ((ev->portev_events & POLLHUP) != 0) {
				/* XXX Read only!  */
				poll_handler->callback(Event::EOS);
				continue;
			}

			HALT(log_) << "Unexpected poll events: " << ev->portev_events;
		}
	}
}
