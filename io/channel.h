/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

#ifndef	IO_CHANNEL_H
#define	IO_CHANNEL_H

class Action;
class Buffer;

class Channel {
protected:
	Channel(void)
	{ }

public:
	virtual ~Channel()
	{ }

	/*
	 * NB:
	 * close does not take an EventCallback.  It can
	 * fail in any meaningful sense, but it absolutely
	 * must leave the Channel in a state in which it
	 * can be deleted, which is what callers care about,
	 * not whether it closed with an error.
	 */
	virtual Action *close(SimpleCallback *) = 0;

private:
	Action *close(EventCallback *);
};

class BlockChannel : public Channel {
protected:
	size_t bsize_;

	BlockChannel(size_t bsize)
	: bsize_(bsize)
	{ }

public:
	virtual ~BlockChannel()
	{ }

	virtual Action *read(off_t, EventCallback *) = 0;
	virtual Action *write(off_t, Buffer *, EventCallback *) = 0;
};

class StreamChannel : public Channel {
protected:
	StreamChannel(void)
	{ }

public:
	virtual ~StreamChannel()
	{ }

	virtual Action *read(size_t, EventCallback *) = 0;
	virtual Action *write(Buffer *, EventCallback *) = 0;

	virtual Action *shutdown(bool, bool, EventCallback *) = 0;
};

#endif /* !IO_CHANNEL_H */
