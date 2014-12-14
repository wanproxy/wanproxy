/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
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

#ifndef	IO_PIPE_PIPE_SIMPLE_WRAPPER_H
#define	IO_PIPE_PIPE_SIMPLE_WRAPPER_H

#include <io/pipe/pipe_producer.h>

template<typename T>
class PipeSimpleWrapper : public PipeProducer {
	typedef bool (T::*process_method_t)(Buffer *, Buffer *);

	T *obj_;
	process_method_t process_method_;
public:
	template<typename Tp>
	PipeSimpleWrapper(T *obj, Tp process_method)
	: PipeProducer("/pipe/simple/wrapper"),
	  obj_(obj),
	  process_method_(process_method)
	{ }

	~PipeSimpleWrapper()
	{ }

	void consume(Buffer *in)
	{
		Buffer out;
		if (!(obj_->*process_method_)(&out, in)) {
			produce_error();
			return;
		}
		if (!out.empty())
			produce(&out);
	}
};

#endif /* !IO_PIPE_PIPE_SIMPLE_WRAPPER_H */
