#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <http/http_protocol.h>

#include <io/net/tcp_server.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>
#include <io/pipe/splice.h>

#include <io/socket/simple_server.h>

#include <ssh/ssh_protocol.h>

namespace {
	static uint8_t zero_padding[255];
}

/*
 * XXX
 * Need to split up instances of algorithms from algorithm
 * descriptions.
 */

class SSHAlgorithmNegotiation;
class SSHKeyExchange;
class SSHServerHostKey;
class SSHEncryption;
class SSHMAC;
class SSHCompression;
class SSHLanguage;

class SSHKeyExchange {
	std::string name_;
protected:
	SSHKeyExchange(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHKeyExchange()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

class SSHServerHostKey {
	std::string name_;
protected:
	SSHServerHostKey(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHServerHostKey()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

class SSHEncryption {
	std::string name_;
protected:
	SSHEncryption(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHEncryption()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

class SSHMAC {
	std::string name_;
protected:
	SSHMAC(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHMAC()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

class SSHCompression {
	std::string name_;
protected:
	SSHCompression(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHCompression()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

class SSHLanguage {
	std::string name_;
protected:
	SSHLanguage(const std::string& xname)
	: name_(xname)
	{ }

public:
	~SSHLanguage()
	{ }

	std::string name(void) const
	{
		return (name_);
	}

	virtual bool input(Buffer *) = 0;
};

class SSHAlgorithmNegotiation {
	std::map<std::string, SSHKeyExchange *> key_exchange_map_;
	std::map<std::string, SSHServerHostKey *> server_host_key_map_;
	std::map<std::string, SSHEncryption *> encryption_client_to_server_map_;
	std::map<std::string, SSHEncryption *> encryption_server_to_client_map_;
	std::map<std::string, SSHMAC *> mac_client_to_server_map_;
	std::map<std::string, SSHMAC *> mac_server_to_client_map_;
	std::map<std::string, SSHCompression *> compression_client_to_server_map_;
	std::map<std::string, SSHCompression *> compression_server_to_client_map_;
	std::map<std::string, SSHLanguage *> language_client_to_server_map_;
	std::map<std::string, SSHLanguage *> language_server_to_client_map_;
public:
	SSHAlgorithmNegotiation(std::vector<SSHKeyExchange *> key_exchange_list,
				std::vector<SSHServerHostKey *> server_host_key_list,
				std::vector<SSHEncryption *> encryption_client_to_server_list,
				std::vector<SSHEncryption *> encryption_server_to_client_list,
				std::vector<SSHMAC *> mac_client_to_server_list,
				std::vector<SSHMAC *> mac_server_to_client_list,
				std::vector<SSHCompression *> compression_client_to_server_list,
				std::vector<SSHCompression *> compression_server_to_client_list,
				std::vector<SSHLanguage *> language_client_to_server_list,
				std::vector<SSHLanguage *> language_server_to_client_list)
	: key_exchange_map_(list_to_map(key_exchange_list)),
	  server_host_key_map_(list_to_map(server_host_key_list)),
	  encryption_client_to_server_map_(list_to_map(encryption_client_to_server_list)),
	  encryption_server_to_client_map_(list_to_map(encryption_server_to_client_list)),
	  mac_client_to_server_map_(list_to_map(mac_client_to_server_list)),
	  mac_server_to_client_map_(list_to_map(mac_server_to_client_list)),
	  compression_client_to_server_map_(list_to_map(compression_client_to_server_list)),
	  compression_server_to_client_map_(list_to_map(compression_server_to_client_list)),
	  language_client_to_server_map_(list_to_map(language_client_to_server_list)),
	  language_server_to_client_map_(list_to_map(language_server_to_client_list))
	{ }

	SSHAlgorithmNegotiation(std::vector<SSHKeyExchange *> key_exchange_list,
				std::vector<SSHServerHostKey *> server_host_key_list,
				std::vector<SSHEncryption *> encryption_list,
				std::vector<SSHMAC *> mac_list,
				std::vector<SSHCompression *> compression_list,
				std::vector<SSHLanguage *> language_list)
	: key_exchange_map_(list_to_map(key_exchange_list)),
	  server_host_key_map_(list_to_map(server_host_key_list)),
	  encryption_client_to_server_map_(list_to_map(encryption_list)),
	  encryption_server_to_client_map_(list_to_map(encryption_list)),
	  mac_client_to_server_map_(list_to_map(mac_list)),
	  mac_server_to_client_map_(list_to_map(mac_list)),
	  compression_client_to_server_map_(list_to_map(compression_list)),
	  compression_server_to_client_map_(list_to_map(compression_list)),
	  language_client_to_server_map_(list_to_map(language_list)),
	  language_server_to_client_map_(list_to_map(language_list))
	{ }

	/* XXX Add a variant that takes only server_host_key_list and fills in suitable defaults.  */

	~SSHAlgorithmNegotiation()
	{ }

	bool input(Buffer *)
	{
		return (false);
	}

private:
	template<typename T>
	std::map<std::string, typename T::value_type> list_to_map(const T& list)
	{
		std::map<std::string, typename T::value_type> map;
		typename T::const_iterator it;

		for (it = list.begin(); it != list.end(); ++it) {
			typename T::value_type p = *it;

			map[p->name()] = p;
		}

		return (map);
	}
};

class SSHTransportPipe : public PipeProducer {
	enum State {
		GetIdentificationString,
		GetPacket
	};

	State state_;
	Buffer input_buffer_;

	/* XXX These parameters are different for receive and send.  */
	size_t block_size_;
	size_t mac_length_;

	SSHAlgorithmNegotiation *algorithm_negotiation_;

	EventCallback *receive_callback_;
	Action *receive_action_;
public:
	SSHTransportPipe(void)
	: PipeProducer("/ssh/transport/pipe"),
	  state_(GetIdentificationString),
	  block_size_(8),
	  mac_length_(0),
	  algorithm_negotiation_(NULL),
	  receive_callback_(NULL),
	  receive_action_(NULL)
	{
		Buffer identification_string("SSH-2.0-WANProxy " + (std::string)log_ + "\r\n");
		produce(&identification_string);
	}

	~SSHTransportPipe()
	{
		ASSERT(receive_callback_ == NULL);
		ASSERT(receive_action_ == NULL);
	}

	Action *receive(EventCallback *cb)
	{
		ASSERT(receive_callback_ == NULL);
		ASSERT(receive_action_ == NULL);

		/*
		 * XXX
		 * This pattern is implemented about three different
		 * ways in the WANProxy codebase, and they are all kind
		 * of awful.  Need a better way to handle code that
		 * needs executed either on the request of the caller
		 * or when data comes in that satisfies a deferred
		 * callback to a caller.
		 */
		receive_callback_ = cb;
		receive_do();

		if (receive_callback_ != NULL)
			return (cancellation(this, &SSHTransportPipe::receive_cancel));

		ASSERT(receive_action_ != NULL);
		Action *a = receive_action_;
		receive_action_ = NULL;
		return (a);
	}

	/*
	 * Because this is primarily for WAN optimization we always use minimal
	 * padding and zero padding.  Quick and dirty.  Perhaps revisit later,
	 * although it makes send() asynchronous unless we add a blocking
	 * RNG interface.
	 */
	void send(Buffer *payload)
	{
		Buffer packet;
		uint8_t padding_len;
		uint32_t packet_len;

		packet_len = sizeof padding_len + payload->length();
		padding_len = 4 + (block_size_ - ((sizeof packet_len + packet_len + 4) % block_size_));
		packet_len += padding_len;

		packet_len = BigEndian::encode(packet_len);
		packet.append(&packet_len);
		packet.append(padding_len);
		packet.append(payload);
		packet.append(zero_padding, padding_len);

		if (mac_length_ != 0)
			NOTREACHED();

		payload->clear();

		produce(&packet);
	}

private:
	void consume(Buffer *in)
	{
		/* XXX XXX XXX */
		if (in->empty()) {
			if (!input_buffer_.empty())
				DEBUG(log_) << "Received EOS with data outstanding.";
			Buffer eos;
			produce(&eos);
			return;
		}

		input_buffer_.append(in);
		in->clear();

		if (state_ == GetIdentificationString) {
			HTTPProtocol::ParseStatus status;

			while (!input_buffer_.empty()) {
				Buffer line;
				status = HTTPProtocol::ExtractLine(&line, &input_buffer_);
				switch (status) {
				case HTTPProtocol::ParseSuccess:
					break;
				case HTTPProtocol::ParseFailure:
					ERROR(log_) << "Invalid line while waiting for identification string.";
					produce_error();
					return;
				case HTTPProtocol::ParseIncomplete:
					/* Wait for more.  */
					return;
				}

				if (!line.prefix("SSH-"))
					continue; /* Next line.  */

				if (!line.prefix("SSH-2.0")) {
					ERROR(log_) << "Unsupported version.";
					produce_error();
					return;
				}

				state_ = GetPacket;
				break;
			}
		}

		if (state_ == GetPacket && receive_callback_ != NULL)
			receive_do();
	}

	void receive_cancel(void)
	{
		if (receive_action_ != NULL) {
			receive_action_->cancel();
			receive_action_ = NULL;
		}

		if (receive_callback_ != NULL) {
			delete receive_callback_;
			receive_callback_ = NULL;
		}
	}

	void receive_do(void)
	{
		ASSERT(receive_action_ == NULL);
		ASSERT(receive_callback_ != NULL);

		if (state_ != GetPacket)
			return;

		while (!input_buffer_.empty()) {
			Buffer packet;
			Buffer mac;
			uint32_t packet_len;
			uint8_t padding_len;
			uint8_t msg;

			if (input_buffer_.length() <= sizeof packet_len) {
				DEBUG(log_) << "Waiting for packet length.";
				return;
			}

			input_buffer_.extract(&packet_len);
			packet_len = BigEndian::decode(packet_len);

			if (packet_len == 0) {
				ERROR(log_) << "Need to handle 0-length packet.";
				produce_error();
				return;
			}

			if (input_buffer_.length() < sizeof packet_len + packet_len + mac_length_) {
				DEBUG(log_) << "Need " << sizeof packet_len + packet_len + mac_length_ << "bytes; have " << input_buffer_.length() << ".";
				return;
			}

			input_buffer_.moveout(&packet, sizeof packet_len, packet_len);
			if (mac_length_ != 0)
				input_buffer_.moveout(&mac, 0, mac_length_);

			padding_len = packet.peek();
			packet.skip(sizeof padding_len);
			if (padding_len != 0) {
				if (packet.length() < padding_len) {
					ERROR(log_) << "Padding too large for packet.";
					produce_error();
					return;
				}
				packet.trim(padding_len);
			}

			if (packet.empty()) {
				ERROR(log_) << "Need to handle empty packet.";
				produce_error();
				return;
			}

			/*
			 * Pass by range to registered handlers for each range.
			 * Unhandled messages go to the receive_callback_, and
			 * the caller can register key exchange mechanisms,
			 * and handle (or discard) whatever they don't handle.
			 *
			 * NB: The caller could do all this, but it's assumed
			 *     that they usually have better things to do.  If
			 *     they register no handlers, they can certainly do
			 *     so by hand.
			 */
			msg = packet.peek();
			if (msg >= SSH::Message::TransportRangeBegin &&
			    msg <= SSH::Message::TransportRangeEnd) {
				DEBUG(log_) << "Using default handler for transport message.";
			} else if (msg >= SSH::Message::AlgorithmNegotiationRangeBegin &&
				   msg <= SSH::Message::AlgorithmNegotiationRangeEnd) {
				if (algorithm_negotiation_ != NULL) {
					if (algorithm_negotiation_->input(&packet))
						continue;
				}
				DEBUG(log_) << "Using default handler for algorithm negotiation message.";
			} else if (msg >= SSH::Message::KeyExchangeMethodRangeBegin &&
				   msg <= SSH::Message::KeyExchangeMethodRangeEnd) {
				DEBUG(log_) << "Using default handler for key exchange method message.";
			} else if (msg >= SSH::Message::UserAuthenticationGenericRangeBegin &&
				   msg <= SSH::Message::UserAuthenticationGenericRangeEnd) {
				DEBUG(log_) << "Using default handler for generic user authentication message.";
			} else if (msg >= SSH::Message::UserAuthenticationMethodRangeBegin &&
				   msg <= SSH::Message::UserAuthenticationMethodRangeEnd) {
				DEBUG(log_) << "Using default handler for user authentication method message.";
			} else if (msg >= SSH::Message::ConnectionProtocolGlobalRangeBegin &&
				   msg <= SSH::Message::ConnectionProtocolGlobalRangeEnd) {
				DEBUG(log_) << "Using default handler for generic connection protocol message.";
			} else if (msg >= SSH::Message::ConnectionChannelRangeBegin &&
				   msg <= SSH::Message::ConnectionChannelRangeEnd) {
				DEBUG(log_) << "Using default handler for connection channel message.";
			} else if (msg >= SSH::Message::ClientProtocolReservedRangeBegin &&
				   msg <= SSH::Message::ClientProtocolReservedRangeEnd) {
				DEBUG(log_) << "Using default handler for client protocol message.";
			} else if (msg >= SSH::Message::LocalExtensionRangeBegin) {
				/* Because msg is a uint8_t, it will always be <= SSH::Message::LocalExtensionRangeEnd.  */
				DEBUG(log_) << "Using default handler for local extension message.";
			} else {
				ASSERT(msg == 0);
				ERROR(log_) << "Message outside of protocol range received.  Passing to default handler, but not expecting much.";
			}

			receive_callback_->param(Event(Event::Done, packet));
			receive_action_ = receive_callback_->schedule();
			receive_callback_ = NULL;

			return;
		}
	}
};

class SSHConnection {
	LogHandle log_;
	Socket *peer_;
	SSHTransportPipe *pipe_;
	Action *receive_action_;
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
public:
	SSHConnection(Socket *peer)
	: log_("/ssh/connection"),
	  peer_(peer),
	  pipe_(NULL),
	  splice_(NULL),
	  splice_action_(NULL),
	  close_action_(NULL)
	{
		pipe_ = new SSHTransportPipe();
		EventCallback *rcb = callback(this, &SSHConnection::receive_complete);
		receive_action_ = pipe_->receive(rcb);

		splice_ = new Splice(log_ + "/splice", peer_, pipe_, peer_);
		EventCallback *cb = callback(this, &SSHConnection::splice_complete);
		splice_action_ = splice_->start(cb);
	}

	~SSHConnection()
	{
		ASSERT(close_action_ == NULL);
		ASSERT(splice_action_ == NULL);
		ASSERT(splice_ == NULL);
		ASSERT(receive_action_ == NULL);
		ASSERT(pipe_ == NULL);
		ASSERT(peer_ == NULL);
	}

private:
	void receive_complete(Event e)
	{
		receive_action_->cancel();
		receive_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			ERROR(log_) << "Unexpected event while waiting for a packet: " << e;
			return;
		}

		ASSERT(!e.buffer_.empty());

		/*
		 * SSH Echo!
		 */
		pipe_->send(&e.buffer_);

		EventCallback *rcb = callback(this, &SSHConnection::receive_complete);
		receive_action_ = pipe_->receive(rcb);
	}

	void close_complete(void)
	{
		close_action_->cancel();
		close_action_ = NULL;

		ASSERT(peer_ != NULL);
		delete peer_;
		peer_ = NULL;

		delete this;
	}

	void splice_complete(Event e)
	{
		splice_action_->cancel();
		splice_action_ = NULL;

		switch (e.type_) {
		case Event::EOS:
			DEBUG(log_) << "Peer exiting normally.";
			break;
		case Event::Error:
			ERROR(log_) << "Peer exiting with error: " << e;
			break;
		default:
			ERROR(log_) << "Peer exiting with unknown event: " << e;
			break;
		}

	        ASSERT(splice_ != NULL);
		delete splice_;
		splice_ = NULL;

		if (receive_action_ != NULL) {
			INFO(log_) << "Peer exiting while waiting for a packet.";

			receive_action_->cancel();
			receive_action_ = NULL;
		}

		ASSERT(pipe_ != NULL);
		delete pipe_;
		pipe_ = NULL;

		ASSERT(close_action_ == NULL);
		SimpleCallback *cb = callback(this, &SSHConnection::close_complete);
		close_action_ = peer_->close(cb);
	}
};

class SSHServer : public SimpleServer<TCPServer> {
public:
	SSHServer(SocketAddressFamily family, const std::string& interface)
	: SimpleServer<TCPServer>("/ssh/server", family, interface)
	{ }

	~SSHServer()
	{ }

	void client_connected(Socket *client)
	{
		new SSHConnection(client);
	}
};


int
main(void)
{
	new SSHServer(SocketAddressFamilyIP, "[::]:2299");
	event_main();
}
