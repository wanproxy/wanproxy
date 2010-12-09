#include <sys/types.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_poll.h>

static std::map<EventPoll *, int> kq_map;

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_()
{
	int kq = kqueue();
	ASSERT(kq != -1);
	kq_map[this] = kq;
}

EventPoll::~EventPoll()
{
	ASSERT(read_poll_.empty());
	ASSERT(write_poll_.empty());

	std::map<EventPoll *, int>::iterator it;
	it = kq_map.find(this);
	ASSERT(it != kq_map.end());
	kq_map.erase(it);
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	ASSERT(fd != -1);

	EventPoll::PollHandler *poll_handler;
	struct kevent kev;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) == read_poll_.end());
		poll_handler = &read_poll_[fd];
		EV_SET(&kev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) == write_poll_.end());
		poll_handler = &write_poll_[fd];
		EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
		break;
	default:
		NOTREACHED();
	}
	int kq = kq_map[this];
	int evcnt = ::kevent(kq, &kev, 1, NULL, 0, NULL);
	if (evcnt == -1)
		HALT(log_) << "Could not add event to kqueue.";
	ASSERT(evcnt == 0);
	ASSERT(poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	struct kevent kev;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		EV_SET(&kev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		EV_SET(&kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		break;
	}
	int kq = kq_map[this];
	int evcnt = ::kevent(kq, &kev, 1, NULL, 0, NULL);
	if (evcnt == -1)
		HALT(log_) << "Could not delete event from kqueue.";
	ASSERT(evcnt == 0);
}

void
EventPoll::wait(int ms)
{
	static const unsigned kevcnt = 128;
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

	struct kevent kev[kevcnt];
	int kq = kq_map[this];
	int evcnt = kevent(kq, NULL, 0, kev, kevcnt, ms == -1 ? NULL : &ts);
	if (evcnt == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not poll kqueue.";
	}

	int i;
	for (i = 0; i < evcnt; i++) {
		struct kevent *ev = &kev[i];
		EventPoll::PollHandler *poll_handler;
		switch (ev->filter) {
		case EVFILT_READ:
			ASSERT(read_poll_.find(ev->ident) != read_poll_.end());
			poll_handler = &read_poll_[ev->ident];
			break;
		case EVFILT_WRITE:
			ASSERT(write_poll_.find(ev->ident) !=
			       write_poll_.end());
			poll_handler = &write_poll_[ev->ident];
			break;
		default:
			NOTREACHED();
		}
		if ((ev->flags & EV_ERROR) != 0) {
			poll_handler->callback(Event(Event::Error, ev->fflags));
			continue;
		}
		if ((ev->flags & EV_EOF) != 0 && ev->filter == EVFILT_READ) {
			poll_handler->callback(Event(Event::EOS, ev->fflags));
			continue;
		}
		/*
		 * XXX
		 * We do not currently have a way to indicate that the reader
		 * has called shutdown and will no longer read data.  We just
		 * indicate Done and let the next write fail.
		 */
		poll_handler->callback(Event(Event::Done, ev->fflags));
	}
}
