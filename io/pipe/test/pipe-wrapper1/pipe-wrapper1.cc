#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_simple.h>
#include <io/pipe/pipe_simple_wrapper.h>

static uint8_t data[65536];

class Producer {
	LogHandle log_;
	TestGroup group_;
	Pipe *pipe_;
	Action *action_;
public:
	Producer(Pipe *pipe)
	: log_("/producer"),
	  group_("/test/io/pipe/wrapper/producer", "Producer"),
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
	  group_("/test/io/pipe/wrapper/consumer", "Consumer"),
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

class Processor {
	unsigned sum_;
public:
	Processor(void)
	: sum_(0)
	{ }

	~Processor()
	{ }

	bool process(Buffer *output, Buffer *input)
	{
		while (!input->empty()) {
			sum_ += input->peek();
			output->append(input->peek());
			input->skip(1);
		}
		return (true);
	}

	unsigned sum(void) const
	{
		return (sum_);
	}
};

int
main(void)
{
	unsigned sum;
	unsigned i;

	sum = 0;
	for (i = 0; i < sizeof data; i++) {
		data[i] = random() % 0xff;
		sum += data[i];
	}

	Processor proc;
	PipeSimpleWrapper<Processor> pipe(&proc, &Processor::process);
	Producer producer(&pipe);
	Consumer consumer(&pipe);

	event_main();

	{
		TestGroup g("/test/io/pipe/wrapper/processor", "Processor");
		Test _(g, "Corrext checksum.");
		if (sum == proc.sum())
			_.pass();
	}
}
