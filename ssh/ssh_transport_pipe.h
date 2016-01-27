/*
 * Copyright (c) 2011-2016 Juli Mallett. All rights reserved.
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

#ifndef	SSH_SSH_TRANSPORT_PIPE_H
#define	SSH_SSH_TRANSPORT_PIPE_H

#include <event/cancellation.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

namespace SSH {
	struct Session;

	/*
	 * XXX
	 * It would be nice to have a Transport base class
	 * for use throughout the codebase to handle things
	 * like sending packets, and have TransportPipe be
	 * a version that's modeled as a Pipe.
	 */
	class TransportPipe : public PipeProducer {
		enum State {
			GetIdentificationString,
			GetPacket
		};

		Mutex mtx_;

		Session *session_;

		State state_;
		Buffer input_buffer_;
		Buffer first_block_;

		Cancellation<TransportPipe> receive_cancel_;
		EventCallback *receive_callback_;
		Action *receive_action_;

		bool ready_;
		Cancellation<TransportPipe> ready_cancel_;
		SimpleCallback *ready_callback_;
		Action *ready_action_;
	public:
		TransportPipe(Session *);
		~TransportPipe();

		Action *receive(EventCallback *);
		void send(Buffer *);

		Action *ready(SimpleCallback *);

		void key_exchange_complete(void);

	private:
		void consume(Buffer *);

		void receive_cancel(void);
		void receive_do(void);

		void ready_cancel(void);
	};
}

#endif /* !SSH_SSH_TRANSPORT_PIPE_H */
