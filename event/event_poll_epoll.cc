#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/event_callback.h>
#include <event/event_poll.h>

#define	EPOLL_EVENT_COUNT	128

struct EventPollState {
	int ep_;
};

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_(),
  state_(new EventPollState())
{
	state_->ep_ = epoll_create(EPOLL_EVENT_COUNT);
	ASSERT(state_->ep_ != -1);
}

EventPoll::~EventPoll()
{
	ASSERT(read_poll_.empty());
	ASSERT(write_poll_.empty());

	if (state_ != NULL) {
		if (state_->ep_ != -1) {
			close(state_->ep_);
			state_->ep_ = -1;
		}
		delete state_;
		state_ = NULL;
	}
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	ASSERT(fd != -1);

	EventPoll::PollHandler *poll_handler;
	struct epoll_event eev;
	bool unique = true;
	eev.data.fd = fd;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) == read_poll_.end());
		unique = write_poll_.find(fd) == write_poll_.end();
		poll_handler = &read_poll_[fd];
		eev.events = EPOLLIN | (unique ? 0 : EPOLLOUT);
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) == write_poll_.end());
		unique = read_poll_.find(fd) == read_poll_.end();
		poll_handler = &write_poll_[fd];
		eev.events = EPOLLOUT | (unique ? 0 : EPOLLIN);
		break;
	default:
		NOTREACHED();
	}
	int rv = ::epoll_ctl(state_->ep_, unique ? EPOLL_CTL_ADD : EPOLL_CTL_MOD, fd, &eev);
	if (rv == -1)
		HALT(log_) << "Could not add event to epoll.";
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

	struct epoll_event eev;
	bool unique = true;
	eev.data.fd = fd;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(read_poll_.find(fd) != read_poll_.end());
		unique = write_poll_.find(fd) == write_poll_.end();
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		eev.events = unique ? 0 : EPOLLOUT;
		break;
	case EventPoll::Writable:
		ASSERT(write_poll_.find(fd) != write_poll_.end());
		unique = read_poll_.find(fd) == read_poll_.end();
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		eev.events = unique ? 0 : EPOLLIN;
		break;
	}
	int rv = ::epoll_ctl(state_->ep_, unique ? EPOLL_CTL_DEL : EPOLL_CTL_MOD, fd, &eev);
	if (rv == -1)
		HALT(log_) << "Could not delete event from epoll.";
	ASSERT(rv == 0);
}

void
EventPoll::wait(int ms)
{
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

	struct epoll_event eev[EPOLL_EVENT_COUNT];
	int evcnt = ::epoll_wait(state_->ep_, eev, EPOLL_EVENT_COUNT, ms);
	if (evcnt == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not poll epoll.";
	}

	int i;
	for (i = 0; i < evcnt; i++) {
		struct epoll_event *ev = &eev[i];
		EventPoll::PollHandler *poll_handler;
		if ((ev->events & EPOLLIN) != 0) {
			ASSERT(read_poll_.find(ev->data.fd) != read_poll_.end());
			poll_handler = &read_poll_[ev->data.fd];
			poll_handler->callback(Event::Done);
		}

		if ((ev->events & EPOLLOUT) != 0) {
			ASSERT(write_poll_.find(ev->data.fd) != write_poll_.end());
			poll_handler = &write_poll_[ev->data.fd];
			poll_handler->callback(Event::Done);
		}

		if ((ev->events & EPOLLIN) == 0 && (ev->events & EPOLLOUT) == 0) {
			if (read_poll_.find(ev->data.fd) != read_poll_.end()) {
				poll_handler = &read_poll_[ev->data.fd];
			} else if (write_poll_.find(ev->data.fd) != write_poll_.end()) {
				poll_handler = &write_poll_[ev->data.fd];

				if ((ev->events & (EPOLLERR | EPOLLHUP)) == EPOLLHUP) {
					DEBUG(log_) << "Got EPOLLHUP on write poll.";
					continue;
				}
			} else {
				HALT(log_) << "Unexpected poll fd.";
				continue;
			}

			if ((ev->events & EPOLLERR) != 0) {
				poll_handler->callback(Event::Error);
				continue;
			}

			if ((ev->events & EPOLLHUP) != 0) {
				poll_handler->callback(Event::EOS);
				continue;
			}

			HALT(log_) << "Unexpected poll events: " << ev->events;
		}
	}
}
