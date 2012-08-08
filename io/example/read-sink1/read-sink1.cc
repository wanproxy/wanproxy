#include <unistd.h>

#include <event/callback_handler.h>
#include <event/event_callback.h>
#include <event/event_handler.h>
#include <event/event_main.h>

#include <io/stream_handle.h>

class Sink {
	LogHandle log_;

	StreamHandle fd_;
	CallbackHandler close_handler_;
	EventHandler read_handler_;
public:
	Sink(int fd)
	: log_("/sink"),
	  fd_(fd),
	  read_handler_()
	{
		read_handler_.handler(Event::Done, this, &Sink::read_done);
		read_handler_.handler(Event::EOS, this, &Sink::read_eos);

		close_handler_.handler(this, &Sink::close_done);

		read_handler_.wait(fd_.read(0, read_handler_.callback()));
	}

	~Sink()
	{ }

	void read_done(Event)
	{
		read_handler_.wait(fd_.read(0, read_handler_.callback()));
	}

	void read_eos(Event)
	{
		close_handler_.wait(fd_.close(close_handler_.callback()));
	}

	void close_done(void)
	{ }
};

int
main(void)
{
	Sink sink(STDIN_FILENO);

	event_main();
}
