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

#include <event/event_callback.h>

#include <io/block_handle.h>
#include <io/io_system.h>

BlockHandle::BlockHandle(int fd, size_t bsize)
: BlockChannel(bsize),
  log_("/block/device"),
  fd_(fd)
{
	int flags = ::fcntl(fd_, F_GETFL, 0);
	if (flags == -1) {
		ERROR(log_) << "Could not get flags for file descriptor.";
	} else {
		flags |= O_NONBLOCK;

		flags = ::fcntl(fd_, F_SETFL, flags);
		if (flags == -1)
			ERROR(log_) << "Could not set flags for file descriptor, some operations may block.";
	}

	IOSystem::instance()->attach(fd_, this);
}

BlockHandle::~BlockHandle()
{
	IOSystem::instance()->detach(fd_, this);
}

Action *
BlockHandle::close(SimpleCallback *cb)
{
	return (IOSystem::instance()->close(fd_, this, cb));
}

Action *
BlockHandle::read(off_t offset, EventCallback *cb)
{
	return (IOSystem::instance()->read(fd_, this, offset * bsize_, bsize_, cb));
}

Action *
BlockHandle::write(off_t offset, Buffer *buffer, EventCallback *cb)
{
	if (buffer->length() != bsize_) {
		cb->param(Event::Error);
		return (cb->schedule());
	}
	return (IOSystem::instance()->write(fd_, this, offset * bsize_, buffer, cb));
}
