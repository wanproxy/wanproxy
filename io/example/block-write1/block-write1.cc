#include <fcntl.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/block_handle.h>

#define	BW_BSIZE	65536
static uint8_t data_buffer[BW_BSIZE];

class BlockWriter {
	LogHandle log_;

	BlockHandle dev_;
	uint64_t block_number_;
	uint64_t block_count_;

	Action *write_action_;
	Action *close_action_;
public:
	BlockWriter(int fd, uint64_t block_count)
	: log_("/blockwriter"),
	  dev_(fd, BW_BSIZE),
	  block_number_(0),
	  block_count_(block_count),
	  write_action_(NULL),
	  close_action_(NULL)
	{
		Buffer buf(data_buffer, sizeof data_buffer);
		EventCallback *cb = callback(this, &BlockWriter::write_complete);
		write_action_ = dev_.write(block_number_, &buf, cb);
	}

	~BlockWriter()
	{
		ASSERT(write_action_ == NULL);
		ASSERT(close_action_ == NULL);
	}

	void write_complete(Event e)
	{
		write_action_->cancel();
		write_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		DEBUG(log_) << "Finished block #" << block_number_ << "/" << block_count_;

		if (++block_number_ == block_count_) {
			EventCallback *cb = callback(this, &BlockWriter::close_complete);
			close_action_ = dev_.close(cb);
			return;
		}

		Buffer buf(data_buffer, sizeof data_buffer);
		EventCallback *cb = callback(this, &BlockWriter::write_complete);
		write_action_ = dev_.write(block_number_, &buf, cb);
	}

	void close_complete(Event e)
	{
		close_action_->cancel();
		close_action_ = NULL;

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
	unsigned i;

	for (i = 0; i < sizeof data_buffer; i++)
		data_buffer[i] = random();

	int fd = ::open("block_file.128M", O_RDWR | O_CREAT);
	ASSERT(fd != -1);
	BlockWriter bw(fd, (128 * 1024 * 1024) / BW_BSIZE);

	event_main();

	::unlink("block_file.128M");
}
