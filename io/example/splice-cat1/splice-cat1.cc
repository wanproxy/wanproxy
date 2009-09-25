#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/pipe.h>
#include <io/pipe_null.h>
#include <io/splice.h>

class Catenate {
	LogHandle log_;

	FileDescriptor input_;
	Action *input_action_;

	FileDescriptor output_;
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
	  splice_(&input_, pipe, &output_)
	{
		EventCallback *cb = callback(this, &Catenate::splice_complete);
		splice_action_ = splice_.start(cb);
	}

	~Catenate()
	{
		ASSERT(input_action_ == NULL);
		ASSERT(output_action_ == NULL);
		ASSERT(splice_action_ == NULL);
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

		ASSERT(e.type_ == Event::EOS);

		EventCallback *icb = callback(this, &Catenate::close_complete, &input_);
		input_action_ = input_.close(icb);

		EventCallback *ocb = callback(this, &Catenate::close_complete, &output_);
		output_action_ = output_.close(ocb);
	}

	void close_complete(Event e, FileDescriptor *fd)
	{
		if (fd == &input_) {
			input_action_->cancel();
			input_action_ = NULL;
		} else if (fd == &output_) {
			output_action_->cancel();
			output_action_ = NULL;
		} else {
			NOTREACHED();
		}

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
	PipeNull pipe;
	Catenate cat(STDIN_FILENO, &pipe, STDOUT_FILENO);

	EventSystem::instance()->start();
}
