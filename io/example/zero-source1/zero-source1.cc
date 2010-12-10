#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/stream_handle.h>

static Buffer zero_buffer;

class Source {
	LogHandle log_;

	StreamHandle fd_;
	Action *action_;
public:
	Source(int fd)
	: log_("/source"),
	  fd_(fd),
	  action_(NULL)
	{
		Buffer tmp(zero_buffer);
		EventCallback *cb = callback(this, &Source::write_complete);
		action_ = fd_.write(&tmp, cb);
	}

	~Source()
	{
		ASSERT(action_ == NULL);
	}

	void write_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::Error:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		if (e.type_ == Event::Error) {
			EventCallback *cb = callback(this, &Source::close_complete);
			action_ = fd_.close(cb);
			return;
		}

		Buffer tmp(zero_buffer);
		EventCallback *cb = callback(this, &Source::write_complete);
		action_ = fd_.write(&tmp, cb);
	}

	void close_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}
	}
};

int
main(void)
{
	static uint8_t zbuf[65536];
	memset(zbuf, 0, sizeof zbuf);

	zero_buffer.append(zbuf, sizeof zbuf);

	Source source(STDOUT_FILENO);

	event_main();
}
