#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/stream_handle.h>
#include <io/pipe/pipe.h>
#include <io/pipe/pipe_sink.h>
#include <io/pipe/splice.h>

class Catenate {
	LogHandle log_;

	StreamHandle input_;
	Action *input_action_;

	StreamHandle output_;
	Action *output_action_;

	Splice splice_;
	Action *splice_action_;
public:
	Catenate(int input, Pipe *pipe, int output)
	: log_("/catenate"),
	  input_(input),
	  input_action_(NULL),
	  output_(output),
	  output_action_(NULL),
	  splice_(log_, &input_, pipe, &output_)
	{
		EventCallback *cb = callback(this, &Catenate::splice_complete);
		splice_action_ = splice_.start(cb);
	}

	~Catenate()
	{
		ASSERT(log_, input_action_ == NULL);
		ASSERT(log_, output_action_ == NULL);
		ASSERT(log_, splice_action_ == NULL);
	}

	void splice_complete(Event e)
	{
		splice_action_->cancel();
		splice_action_ = NULL;

		switch (e.type_) {
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		ASSERT(log_, e.type_ == Event::EOS);

		SimpleCallback *icb = callback(this, &Catenate::close_complete, &input_);
		input_action_ = input_.close(icb);

		SimpleCallback *ocb = callback(this, &Catenate::close_complete, &output_);
		output_action_ = output_.close(ocb);
	}

	void close_complete(StreamHandle *fd)
	{
		if (fd == &input_) {
			input_action_->cancel();
			input_action_ = NULL;
		} else if (fd == &output_) {
			output_action_->cancel();
			output_action_ = NULL;
		} else {
			NOTREACHED(log_);
		}
	}
};

int
main(void)
{
	PipeSink pipe;
	Catenate cat(STDIN_FILENO, &pipe, STDOUT_FILENO);

	event_main();
}
