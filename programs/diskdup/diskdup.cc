/*
 * Copyright (c) 2012 Juli Mallett. All rights reserved.
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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/block_handle.h>

class DiskDup {
	LogHandle log_;
	BlockHandle *source_;
	BlockHandle *target_;
	uint64_t block_number_;
	Action *source_action_;
	Action *target_action_;
	Buffer source_buffer_;
	Buffer target_buffer_;
public:
	DiskDup(BlockHandle *source, BlockHandle *target)
	: log_("/diskdup"),
	  source_(source),
	  target_(target),
	  block_number_(0),
	  source_action_(NULL),
	  target_action_(NULL),
	  source_buffer_(),
	  target_buffer_()
	{
		schedule_read();
	}

private:
	void close_complete(bool target)
	{
		if (target) {
			ASSERT(log_, target_action_ != NULL);
			target_action_->cancel();
			target_action_ = NULL;

			delete target_;
			target_ = NULL;
		} else {
			ASSERT(log_, source_action_ != NULL);
			source_action_->cancel();
			source_action_ = NULL;

			delete source_;
			source_ = NULL;
		}

		if (source_ == NULL && target_ == NULL)
			delete this;
	}

	void read_complete(Event e, bool target)
	{
		if (target) {
			ASSERT(log_, target_action_ != NULL);
			target_action_->cancel();
			target_action_ = NULL;
		} else {
			ASSERT(log_, source_action_ != NULL);
			source_action_->cancel();
			source_action_ = NULL;
		}

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::EOS:
			if (!e.buffer_.empty()) {
				ERROR(log_) << "Received non-empty end-of-stream: " << e;
				INFO(log_) << "Block size may be incorrect.";
			}
			schedule_close();
			return;
		case Event::Error:
			ERROR(log_) << "Received error: " << e;
			schedule_close();
			return;
		default:
			HALT(log_) << "Unexpected event: " << e;
		}

		if (target) {
			target_buffer_ = e.buffer_;
			if (source_buffer_.empty())
				return;
		} else {
			source_buffer_ = e.buffer_;
			if (target_buffer_.empty())
				return;
		}

		ASSERT(log_, source_action_ == NULL && target_action_ == NULL);
		if (!source_buffer_.equal(&target_buffer_)) {
			DEBUG(log_) << "Non-identical block, performing write.";
			schedule_write();
			return;
		}
		DEBUG(log_) << "Identical block, incrementing block number.";
		block_number_++;
		schedule_read();
	}

	void write_complete(Event e)
	{
		ASSERT(log_, target_action_ != NULL);
		target_action_->cancel();
		target_action_ = NULL;

		ASSERT(log_, source_action_ == NULL);

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			ERROR(log_) << "Unexpected event during write: " << e;
			schedule_close();
			return;
		}

		DEBUG(log_) << "Write complete, incrementing block number.";
		block_number_++;
		schedule_read();
	}

	void schedule_close(void)
	{
		if (source_action_ != NULL) {
			DEBUG(log_) << "Canceling source action for close.";
			source_action_->cancel();
			source_action_ = NULL;
		}

		if (target_action_ != NULL) {
			DEBUG(log_) << "Canceling target action for close.";
			target_action_->cancel();
			target_action_ = NULL;
		}


		SimpleCallback *scb = callback(this, &DiskDup::close_complete, false);
		source_action_ = source_->close(scb);

		SimpleCallback *tcb = callback(this, &DiskDup::close_complete, true);
		target_action_ = target_->close(tcb);
	}

	void schedule_read(void)
	{
		source_buffer_.clear();
		target_buffer_.clear();

		ASSERT(log_, source_action_ == NULL);
		EventCallback *scb = callback(this, &DiskDup::read_complete, false);
		source_action_ = source_->read(block_number_, scb);

		ASSERT(log_, target_action_ == NULL);
		EventCallback *tcb = callback(this, &DiskDup::read_complete, true);
		target_action_ = target_->read(block_number_, tcb);
	}

	void schedule_write(void)
	{
		ASSERT(log_, source_action_ == NULL);
		ASSERT(log_, target_action_ == NULL);
		EventCallback *cb = callback(this, &DiskDup::write_complete);
		target_action_ = target_->write(block_number_, &source_buffer_, cb);
	}
};

static BlockHandle *open_block_device(const char *, bool, size_t);
static void usage(void);

int
main(int argc, char *argv[])
{
	size_t block_size;
	int ch;

	block_size = 65536;

	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch (ch) {
		case 's':
			block_size = atoi(optarg);
			if (block_size == 0)
				usage();
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	BlockHandle *source = open_block_device(argv[0], false, block_size);
	if (source == NULL) {
		ERROR("/diskdup") << "Could not open source volume.";
		return (1);
	}

	BlockHandle *target = open_block_device(argv[1], true, block_size);
	if (target == NULL) {
		ERROR("/diskdup") << "Could not open target volume.";
		return (1);
	}

	new DiskDup(source, target);

	event_main();
}

static BlockHandle *
open_block_device(const char *name, bool write, size_t block_size)
{
	int fd;

	fd = open(name, write ? O_RDWR : O_RDONLY);
	if (fd == -1) {
		ERROR("/diskdup") << "Could not open block device " << name << ": " << strerror(errno);
		return (NULL);
	}

	return (new BlockHandle(fd, block_size));
}

static void
usage(void)
{
	INFO("/usage") <<
"usage: diskdup [-s block_size] source_volume target_volume" << std::endl;
	exit(1);
}
