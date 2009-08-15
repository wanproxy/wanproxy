#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/pipe.h>
#include <io/pipe_null.h>

class Catenate {
	LogHandle log_;

	FileDescriptor input_;
	Action *input_action_;

	Pipe *pipe_;

	FileDescriptor output_;
	Action *output_action_;
public:
	Catenate(int input, Pipe *pipe, int output)
	: log_("/catenate"),
	  input_(input),
	  input_action_(NULL),
	  pipe_(pipe),
	  output_(output),
	  output_action_(NULL)
	{
		EventCallback *cb = callback(this, &Catenate::read_complete);
		input_action_ = input_.read(0, cb);
	}

	~Catenate()
	{
		ASSERT(input_action_ == NULL);
		ASSERT(output_action_ == NULL);
	}

	void read_complete(Event e)
	{
		input_action_->cancel();
		input_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		EventCallback *cb = callback(this, &Catenate::input_complete);
		input_action_ = pipe_->input(&e.buffer_, cb);
	}

	void input_complete(Event e)
	{
		input_action_->cancel();
		input_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		EventCallback *cb = callback(this, &Catenate::output_complete);
		output_action_ = pipe_->output(cb);
	}

	void output_complete(Event e)
	{
		output_action_->cancel();
		output_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		if (!e.buffer_.empty()) {
			EventCallback *cb = callback(this, &Catenate::write_complete);
			output_action_ = output_.write(&e.buffer_, cb);
		} else {
			ASSERT(e.type_ == Event::EOS);

			EventCallback *icb = callback(this, &Catenate::close_complete, &input_);
			input_action_ = input_.close(icb);

			EventCallback *ocb = callback(this, &Catenate::close_complete, &output_);
			output_action_ = output_.close(ocb);
		}
	}

	void write_complete(Event e)
	{
		output_action_->cancel();
		output_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		EventCallback *cb = callback(this, &Catenate::read_complete);
		input_action_ = input_.read(0, cb);
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