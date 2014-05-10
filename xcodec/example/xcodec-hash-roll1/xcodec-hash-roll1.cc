/*
 * Copyright (c) 2012-2013 Juli Mallett. All rights reserved.
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

#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/stream_handle.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_hash.h>

class Sink {
	LogHandle log_;

	StreamHandle fd_;
	XCodecHash hash_;
	unsigned length_;
	Action *action_;
public:
	Sink(int fd)
	: log_("/sink"),
	  fd_(fd),
	  hash_(),
	  length_(0),
	  action_(NULL)
	{
		EventCallback *cb = callback(this, &Sink::read_complete);
		action_ = fd_.read(0, cb);
	}

	~Sink()
	{
		ASSERT(log_, action_ == NULL);
	}

	void read_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		while (!e.buffer_.empty()) {
			BufferSegment *seg;
			e.buffer_.moveout(&seg);

			const uint8_t *p, *q = seg->end();
			if (length_ == XCODEC_SEGMENT_LENGTH) {
				for (p = seg->data(); p < q; p++) {
					hash_.roll(*p);
				}
			} else {
				for (p = seg->data(); p < q; p++) {
					if (length_ == XCODEC_SEGMENT_LENGTH) {
						hash_.roll(*p);
					} else {
						hash_.add(*p);
						length_++;
					}
				}
			}
			seg->unref();
		}

		if (e.type_ == Event::EOS) {
			SimpleCallback *cb = callback(this, &Sink::close_complete);
			action_ = fd_.close(cb);
			return;
		}

		EventCallback *cb = callback(this, &Sink::read_complete);
		action_ = fd_.read(0, cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;
	}
};

int
main(void)
{
	Sink sink(STDIN_FILENO);

	event_main();
}
