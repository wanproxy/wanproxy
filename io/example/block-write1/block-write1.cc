/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <fcntl.h>
#include <unistd.h>

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
		ASSERT(log_, write_action_ == NULL);
		ASSERT(log_, close_action_ == NULL);
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
			SimpleCallback *cb = callback(this, &BlockWriter::close_complete);
			close_action_ = dev_.close(cb);
			return;
		}

		Buffer buf(data_buffer, sizeof data_buffer);
		EventCallback *cb = callback(this, &BlockWriter::write_complete);
		write_action_ = dev_.write(block_number_, &buf, cb);
	}

	void close_complete(void)
	{
		close_action_->cancel();
		close_action_ = NULL;
	}
};

int
main(void)
{
	unsigned i;

	for (i = 0; i < sizeof data_buffer; i++)
		data_buffer[i] = random();

	int fd = ::open("block_file.128M", O_RDWR | O_CREAT, 0700);
	ASSERT("/example/io/block/write1", fd != -1);
	BlockWriter bw(fd, (128 * 1024 * 1024) / BW_BSIZE);

	event_main();

	::unlink("block_file.128M");
}
