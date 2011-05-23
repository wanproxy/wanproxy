#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/stream_handle.h>

class Sink {
	LogHandle log_;

	StreamHandle fd_;
	Action *action_;
public:
	Sink(int fd)
	: log_("/sink"),
	  fd_(fd),
	  action_(NULL)
	{
		EventCallback *cb = callback(this, &Sink::read_complete);
		action_ = fd_.read(0, cb);
	}

	~Sink()
	{
		ASSERT(action_ == NULL);
	}

	void read_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		if (e.type_ == Event::EOS) {
			SimpleCallback *cb = callback(this, &Sink::close_complete);
			action_ = fd_.close(cb);
			return;
		}

		EventCallback *cb = callback(this, &Sink::read_complete);
		action_ = fd_.read(0, cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;
	}
};

int
main(void)
{
	Sink sink(STDIN_FILENO);

	event_main();
}
