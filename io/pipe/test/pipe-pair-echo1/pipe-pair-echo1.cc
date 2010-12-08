#include <common/buffer.h>
#include <common/test.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_pair.h>
#include <io/pipe/pipe_pair_echo.h>

static uint8_t data[65536];

class Producer {
	LogHandle log_;
	TestGroup group_;
	Pipe *pipe_;
	Action *action_;
public:
	Producer(Pipe *pipe)
	: log_("/producer"),
	  group_("/test/io/pipe/null/producer", "Producer"),
	  pipe_(pipe),
	  action_(NULL)
	{
		Buffer buf(data, sizeof data);

		EventCallback *cb = callback(this, &Producer::input_complete);
		action_ = pipe_->input(&buf, cb);

		{
			Test _(group_, "Input method consumed Buffer");
			if (buf.empty())
				_.pass();
		}
	}

	~Producer()
	{
		{
			Test _(group_, "No pending action");
			if (action_ == NULL)
				_.pass();
			else {
				action_->cancel();
				action_ = NULL;
			}
		}
	}

	void input_complete(Event e)
	{
		{
			Test _(group_, "Non-NULL Action");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}

		{
			Test _(group_, "Expected event");
			if (e.type_ == Event::Done)
				_.pass();
		}
	}
};

class Consumer {
	LogHandle log_;
	TestGroup group_;
	Pipe *pipe_;
	Action *action_;
	Buffer buffer_;
public:
	Consumer(Pipe *pipe)
	: log_("/consumer"),
	  group_("/test/io/pipe/null/consumer", "Consumer"),
	  pipe_(pipe),
	  action_(NULL),
	  buffer_()
	{
		EventCallback *cb = callback(this, &Consumer::output_complete);
		action_ = pipe_->output(cb);
	}

	~Consumer()
	{
		{
			Test _(group_, "No pending action");
			if (action_ == NULL)
				_.pass();
			else {
				action_->cancel();
				action_ = NULL;
			}
		}

		{
			Test _(group_, "Got expected data");
			if (buffer_.equal(data, sizeof data))
				_.pass();
		}
	}

	void output_complete(Event e)
	{
		{
			Test _(group_, "Non-NULL Action");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}

		{
			Test _(group_, "Expected event");
			if (e.type_ == Event::Done)
				_.pass();
		}

		{
			Test _(group_, "Non-empty Buffer");
			if (!e.buffer_.empty()) {
				buffer_.append(e.buffer_);

				_.pass();
			}
		}
	}
};

int
main(void)
{
	unsigned i;

	for (i = 0; i < sizeof data; i++)
		data[i] = random() % 0xff;

	PipePairEcho pipe_pair;
	Producer producer(pipe_pair.get_incoming());
	Consumer consumer(pipe_pair.get_outgoing());
	EventSystem::instance()->start();
}
