/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#ifndef	IO_IO_SYSTEM_H
#define	IO_IO_SYSTEM_H

#include <map>

#include <common/thread/mutex.h>

#include <io/io_system.h>

class CallbackScheduler;
class CallbackThread;
class Channel;

class IOSystem {
	struct Handle {
		LogHandle log_;

		Mutex mtx_;
		CallbackScheduler *scheduler_;

		int fd_;
		Channel *owner_;

		off_t read_offset_;
		size_t read_amount_;
		Buffer read_buffer_;
		EventCallback *read_callback_;
		Action *read_action_;

		off_t write_offset_;
		Buffer write_buffer_;
		EventCallback *write_callback_;
		Action *write_action_;

		Handle(CallbackScheduler *, int, Channel *);
		~Handle();

		Action *close_do(SimpleCallback *);

		void read_callback(Event);
		void read_cancel(void);
		Action *read_do(void);
		Action *read_schedule(void);

		void write_callback(Event);
		void write_cancel(void);
		Action *write_do(void);
		Action *write_schedule(void);
	};

	/*
	 * The map is keyed by fd and channel to prevent collision
	 * on the fd with a common race on close.  This is also why
	 * we pass the fd and channel to all functions.  There is
	 * no interaction, it's just a disambiguator.
	 */
	typedef std::pair<int, Channel *> handle_key_t;
	typedef std::map<handle_key_t, Handle *> handle_map_t;

	LogHandle log_;
	Mutex mtx_;
	handle_map_t handle_map_;
	CallbackThread *handler_thread_;

	IOSystem(void);
	~IOSystem();

public:
	void attach(int, Channel *);
	void detach(int, Channel *);

	Action *close(int, Channel *, SimpleCallback *);
	Action *read(int, Channel *, off_t, size_t, EventCallback *);
	Action *write(int, Channel *, off_t, Buffer *, EventCallback *);

	static IOSystem *instance(void)
	{
		static IOSystem *instance_;

		if (instance_ == NULL)
			instance_ = new IOSystem();
		return (instance_);
	}
};

#endif /* !IO_IO_SYSTEM_H */
