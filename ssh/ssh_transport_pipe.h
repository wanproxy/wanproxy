#ifndef	SSH_TRANSPORT_PIPE_H
#define	SSH_TRANSPORT_PIPE_H

namespace SSH {
	class AlgorithmNegotiation;

	class TransportPipe : public PipeProducer {
		enum State {
			GetIdentificationString,
			GetPacket
		};

		State state_;
		Buffer input_buffer_;

		/* XXX These parameters are different for receive and send.  */
		size_t block_size_;
		size_t mac_length_;

		AlgorithmNegotiation *algorithm_negotiation_;

		EventCallback *receive_callback_;
		Action *receive_action_;
	public:
		TransportPipe(AlgorithmNegotiation * = NULL);
		~TransportPipe();

		Action *receive(EventCallback *);
		void send(Buffer *);

	private:
		void consume(Buffer *);

		void receive_cancel(void);
		void receive_do(void);
	};
}

#endif /* !SSH_TRANSPORT_PIPE_H */
