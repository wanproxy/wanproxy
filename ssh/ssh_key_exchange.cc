#include <common/buffer.h>

#include <openssl/dh.h>

#include <event/event_callback.h>

#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_transport_pipe.h>

#define	DH_GROUP_MIN	1024
#define	DH_GROUP_MAX	8192

namespace {
	static const uint8_t
		DiffieHellmanGroupExchangeRequest = 34,
		DiffieHellmanGroupExchangeGroup = 31,
		DiffieHellmanGroupExchangeInitialize = 32,
		DiffieHellmanGroupExchangeReply = 33;

	class DiffieHellmanGroupExchange : public SSH::KeyExchange {
		LogHandle log_;
		DH *dh_;
	public:
		DiffieHellmanGroupExchange(void)
		: SSH::KeyExchange("diffie-hellman-group-exchange-sha1"),
		  log_("/ssh/key_exchange/stub"),
		  dh_(NULL)
		{ }

		~DiffieHellmanGroupExchange()
		{ }

		bool input(SSH::TransportPipe *pipe, Buffer *in)
		{
			uint32_t max, min, n;
			Buffer packet;

			switch (in->peek()) {
			case DiffieHellmanGroupExchangeRequest:
				in->skip(1);
				if (!SSH::UInt32::decode(&min, in))
					return (false);
				if (!SSH::UInt32::decode(&n, in))
					return (false);
				if (!SSH::UInt32::decode(&max, in))
					return (false);
				if (min < DH_GROUP_MIN)
					min = DH_GROUP_MIN;
				if (max > DH_GROUP_MAX)
					max = DH_GROUP_MAX;
				if (min > max)
					return (false);
				if (n < min)
					n = min;
				else if (n > max)
					n = max;

				DEBUG(log_) << "Doing DH_generate_parameters for " << n << " bits.";
				ASSERT(log_, dh_ == NULL);
				dh_ = DH_generate_parameters(n, 2, NULL, NULL);
				if (dh_ == NULL)
					return (false);

				packet.append(DiffieHellmanGroupExchangeGroup);
				SSH::MPInt::encode(&packet, dh_->p);
				SSH::MPInt::encode(&packet, dh_->g);
				DEBUG(log_) << "Sending DH group:" << std::endl << packet.hexdump();
				pipe->send(&packet);
				return (true);
			default:
				ERROR(log_) << "Not yet implemented.";
				return (false);
			}
		}
	};
}

SSH::KeyExchange *
SSH::KeyExchange::method(void)
{
	return (new DiffieHellmanGroupExchange());
}
