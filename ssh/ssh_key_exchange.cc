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

	/*
	 * XXX
	 * Like a non-trivial amount of other code, this has been
	 * written a bit fast-and-loose.  The usage of the dh_ and
	 * k_ in particularly are a bit dodgy and need to be freed
	 * in the destructor.
	 *
	 * Need to add assertions and frees.
	 */
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

		KeyExchange *clone(void) const
		{
			return (new DiffieHellmanGroupExchange(session_));
		}

		bool hash(Buffer *out, const Buffer *in) const
		{
			return (CryptoHash::hash(CryptoHash::SHA1, out, in));
		}

		bool input(SSH::TransportPipe *pipe, Buffer *in)
		{
			SSH::ServerHostKey *key;
			uint32_t max, min, n;
			BIGNUM *e, *f;
			Buffer server_public_key;
			Buffer signature;
			Buffer packet;
			Buffer group;
			Buffer exchange_hash;
			Buffer data;
			Buffer initialize;

			switch (in->peek()) {
			case DiffieHellmanGroupExchangeRequest:
				if (session_->role_ != SSH::ServerRole) {
					ERROR(log_) << "Received group exchange request as client.";
					return (false);
				}
				in->skip(1);
				key_exchange_ = *in;
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
				if (dh_ == NULL) {
					ERROR(log_) << "DH_generate_parameters failed.";
					return (false);
				}

				SSH::MPInt::encode(&group, dh_->p);
				SSH::MPInt::encode(&group, dh_->g);
				key_exchange_.append(group);

				packet.append(DiffieHellmanGroupExchangeGroup);
				packet.append(group);
				pipe->send(&packet);
				return (true);
			case DiffieHellmanGroupExchangeGroup:
				if (session_->role_ != SSH::ClientRole) {
					ERROR(log_) << "Received DH group as server.";
					return (false);
				}
				in->skip(1);
				key_exchange_.append(in);

				dh_ = DH_new();
				if (dh_ == NULL) {
					ERROR(log_) << "DH_new failed.";
					return (false);
				}

				if (!SSH::MPInt::decode(&dh_->p, in))
					return (false);
				if (!SSH::MPInt::decode(&dh_->g, in))
					return (false);

				if (!DH_generate_key(dh_)) {
					ERROR(log_) << "DH_generate_key failed.";
					return (false);
				}
				e = dh_->pub_key;

				SSH::MPInt::encode(&initialize, e);
				key_exchange_.append(initialize);

				packet.append(DiffieHellmanGroupExchangeInitialize);
				packet.append(initialize);
				pipe->send(&packet);
				return (true);
			case DiffieHellmanGroupExchangeInitialize:
				if (session_->role_ != SSH::ServerRole) {
					ERROR(log_) << "Received group exchange initialization as client.";
					return (false);
				}
				in->skip(1);
				key_exchange_.append(in);
				if (!SSH::MPInt::decode(&e, in))
					return (false);

				if (!DH_generate_key(dh_))
					return (false);
				f = dh_->pub_key;

				SSH::MPInt::encode(&key_exchange_, f);
				if (!exchange_finish(e)) {
					ERROR(log_) << "Server key exchange finish failed.";
					return (false);
				}

				key = session_->chosen_algorithms_.server_host_key_;
				if (!key->sign(&signature, &session_->exchange_hash_))
					return (false);
				key->encode_public_key(&server_public_key);

				packet.append(DiffieHellmanGroupExchangeReply);
				SSH::String::encode(&packet, server_public_key);
				SSH::MPInt::encode(&packet, f);
				SSH::String::encode(&packet, &signature);
				pipe->send(&packet);

				/*
				 * XXX
				 * Should send NEWKEYS.
				 */
				return (true);
			case DiffieHellmanGroupExchangeReply:
				if (session_->role_ != SSH::ClientRole) {
					ERROR(log_) << "Received group exchange reply as client.";
					return (false);
				}
				in->skip(1);
				if (!SSH::String::decode(&server_public_key, in))
					return (false);
				if (!SSH::MPInt::decode(&f, in))
					return (false);
				if (!SSH::String::decode(&signature, in))
					return (false);

				key = session_->chosen_algorithms_.server_host_key_;
				if (!key->decode_public_key(&server_public_key)) {
					ERROR(log_) << "Could not decode server public key:" << std::endl << server_public_key.hexdump();
					return (false);
				}

				SSH::MPInt::encode(&key_exchange_, f);
				if (!exchange_finish(f)) {
					ERROR(log_) << "Client key exchange finish failed.";
					return (false);
				}

				if (!key->verify(&signature, &session_->exchange_hash_)) {
					ERROR(log_) << "Failed to verify exchange hash.";
					return (false);
				}

				/*
				 * XXX
				 * Should send NEWKEYS, but we're not ready for that yet.
				 * For now we just assume the peer will do it.  How lazy,
				 * no?
				 */
				return (true);
			default:
				ERROR(log_) << "Not yet implemented.";
				return (false);
			}
		}

		bool init(Buffer *out)
		{
			ASSERT(log_, out->empty());
			ASSERT(log_, session_->role_ == SSH::ClientRole);

			Buffer request;
			SSH::UInt32::encode(&request, DH_GROUP_MIN);
			SSH::UInt32::encode(&request, DH_GROUP_MIN);
			SSH::UInt32::encode(&request, DH_GROUP_MAX);

			key_exchange_ = request;

			out->append(DiffieHellmanGroupExchangeRequest);
			out->append(request);

			return (true);
		}

	private:
		bool exchange_finish(BIGNUM *remote_pubkey)
		{
			SSH::ServerHostKey *key;
			Buffer server_public_key;
			Buffer exchange_hash;
			Buffer data;

			ASSERT(log_, dh_ != NULL);

			uint8_t secret[DH_size(dh_)];
			int secretlen = DH_compute_key(secret, remote_pubkey, dh_);
			if (secretlen == -1)
				return (false);
			k_ = BN_bin2bn(secret, secretlen, NULL);
			if (k_ == NULL)
				return (false);

			key = session_->chosen_algorithms_.server_host_key_;
			key->encode_public_key(&server_public_key);

			SSH::String::encode(&data, session_->client_version_);
			SSH::String::encode(&data, session_->server_version_);
			SSH::String::encode(&data, session_->client_kexinit_);
			SSH::String::encode(&data, session_->server_kexinit_);
			SSH::String::encode(&data, server_public_key);
			data.append(key_exchange_);
			SSH::MPInt::encode(&data, k_);

			if (!CryptoHash::hash(CryptoHash::SHA1, &exchange_hash, &data))
				return (false);

			session_->exchange_hash_ = exchange_hash;
			SSH::MPInt::encode(&session_->shared_secret_, k_);
			if (session_->session_id_.empty())
				session_->session_id_ = exchange_hash;

			return (true);
		}
	};
}

SSH::KeyExchange *
SSH::KeyExchange::method(SSH::Session *session)
{
	return (new DiffieHellmanGroupExchange(session));
}
