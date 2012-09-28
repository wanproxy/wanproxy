#include <openssl/bn.h>
#include <openssl/dh.h>

#include <common/buffer.h>

#include <crypto/crypto_hash.h>

#include <event/event_callback.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_server_host_key.h>
#include <ssh/ssh_session.h>
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
		SSH::Session *session_;
		DH *dh_;
		Buffer key_exchange_;
		BIGNUM *k_;
	public:
		DiffieHellmanGroupExchange(SSH::Session *session)
		: SSH::KeyExchange("diffie-hellman-group-exchange-sha1"),
		  log_("/ssh/key_exchange/stub"),
		  session_(session),
		  dh_(NULL),
		  key_exchange_(),
		  k_()
		{ }

		~DiffieHellmanGroupExchange()
		{ }

		bool input(SSH::TransportPipe *pipe, Buffer *in)
		{
			SSH::ServerHostKey *key;
			uint32_t max, min, n;
			BIGNUM *e, *f;
			Buffer server_public_key;
			Buffer signature;
			Buffer packet;
			Buffer group;
			Buffer hash;
			Buffer data;
			Buffer buf;

			switch (in->peek()) {
			case DiffieHellmanGroupExchangeRequest:
				in->skip(1);
				key_exchange_.append(in);
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

				SSH::MPInt::encode(&group, dh_->p);
				SSH::MPInt::encode(&group, dh_->g);
				key_exchange_.append(group);

				packet.append(DiffieHellmanGroupExchangeGroup);
				packet.append(group);
				pipe->send(&packet);
				return (true);
			case DiffieHellmanGroupExchangeInitialize:
				in->skip(1);
				key_exchange_.append(in);
				if (!SSH::MPInt::decode(&e, in))
					return (false);

				if (!DH_generate_key(dh_))
					return (false);
				f = dh_->pub_key;

				{
					uint8_t secret[DH_size(dh_)];
					int secretlen = DH_compute_key(secret, e, dh_);
					if (secretlen == -1)
						return (false);
					k_ = BN_bin2bn(secret, secretlen, NULL);
					if (k_ == NULL)
						return (false);
					session_->shared_secret_ = Buffer(secret, secretlen);
				}

				key = session_->algorithm_negotiation_->chosen_server_host_key();
				key->encode_public_key(&server_public_key);

				SSH::String::encode(&buf, server_public_key);
				SSH::MPInt::encode(&buf, f);
				key_exchange_.append(buf);
				SSH::MPInt::encode(&key_exchange_, k_);

				SSH::String::encode(&data, session_->client_version_);
				SSH::String::encode(&data, session_->server_version_);
				SSH::String::encode(&data, session_->client_kexinit_);
				SSH::String::encode(&data, session_->server_kexinit_);
				SSH::String::encode(&data, server_public_key);
				data.append(key_exchange_);
				SSH::String::encode(&data, session_->shared_secret_);

				if (!CryptoHash::hash(CryptoHash::SHA1, &hash, &data))
					return (false);

				if (!key->sign(&signature, &hash))
					return (false);
				SSH::String::encode(&buf, &signature);

				packet.append(DiffieHellmanGroupExchangeReply);
				packet.append(buf);
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
SSH::KeyExchange::method(SSH::Session *session)
{
	return (new DiffieHellmanGroupExchange(session));
}
