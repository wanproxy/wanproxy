#ifndef	SSH_SSH_TRANSPORT_PIPE_H
#define	SSH_SSH_TRANSPORT_PIPE_H

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

		Session *session_;

		State state_;
		Buffer input_buffer_;
		Buffer first_block_;

		EventCallback *receive_callback_;
		Action *receive_action_;

		bool ready_;
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
