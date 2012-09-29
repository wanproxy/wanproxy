#ifndef	SSH_SSH_SESSION_H
#define	SSH_SSH_SESSION_H

namespace SSH {
	class AlgorithmNegotiation;

	enum Role {
		ClientRole,
		ServerRole,
	};

	struct UnidirectionalAlgorithms {
		Encryption *encryption_;
		MAC *mac_;
		Compression *compression_;
		Language *language_;

		UnidirectionalAlgorithms(void)
		: encryption_(NULL),
		  mac_(NULL),
		  compression_(NULL),
		  language_(NULL)
		{ }
	};

	struct Algorithms {
		KeyExchange *key_exchange_;
		ServerHostKey *server_host_key_;
		UnidirectionalAlgorithms client_to_server_;
		UnidirectionalAlgorithms server_to_client_;
		UnidirectionalAlgorithms *local_to_remote_;
		UnidirectionalAlgorithms *remote_to_local_;

		Algorithms(Role role)
		: key_exchange_(NULL),
		  server_host_key_(NULL),
		  client_to_server_(),
		  server_to_client_(),
		  local_to_remote_(role == ClientRole ? &client_to_server_ : &server_to_client_),
		  remote_to_local_(role == ClientRole ? &server_to_client_ : &client_to_server_)
		{ }
	};

	struct Session {
		Role role_;

		AlgorithmNegotiation *algorithm_negotiation_;
		Algorithms chosen_algorithms_;
		Algorithms active_algorithms_;
		Buffer client_version_;	/* Client's version string.  */
		Buffer server_version_;	/* Server's version string.  */
		Buffer client_kexinit_;	/* Client's first key exchange packet.  */
		Buffer server_kexinit_;	/* Server's first key exchange packet.  */
		Buffer shared_secret_;	/* Shared secret from key exchange.  */
		Buffer session_id_;	/* First exchange hash.  */
		Buffer exchange_hash_;	/* Most recent exchange hash.  */
		Buffer client_to_server_iv_;	/* Initial client-to-server IV.  */
		Buffer server_to_client_iv_;	/* Initial server-to-client IV.  */
		Buffer client_to_server_key_;	/* Client-to-server encryption key.  */
		Buffer server_to_client_key_;	/* Server-to-client encryption key.  */
		Buffer client_to_server_integrity_key_;	/* Client-to-server integrity key.  */
		Buffer server_to_client_integrity_key_;	/* Server-to-client integrity key.  */

		Session(Role role)
		: role_(role),
		  algorithm_negotiation_(NULL),
		  chosen_algorithms_(role),
		  active_algorithms_(role),
		  client_version_(),
		  server_version_(),
		  client_kexinit_(),
		  server_kexinit_(),
		  shared_secret_(),
		  session_id_(),
		  exchange_hash_(),
		  client_to_server_iv_(),
		  server_to_client_iv_(),
		  client_to_server_key_(),
		  server_to_client_key_(),
		  client_to_server_integrity_key_(),
		  server_to_client_integrity_key_()
		{ }

		void local_version(const Buffer& version)
		{
			if (role_ == ClientRole)
				client_version_ = version;
			else
				server_version_ = version;
		}

		void remote_version(const Buffer& version)
		{
			if (role_ == ClientRole)
				server_version_ = version;
			else
				client_version_ = version;
		}

		void local_kexinit(const Buffer& kexinit)
		{
			if (role_ == ClientRole)
				client_kexinit_ = kexinit;
			else
				server_kexinit_ = kexinit;
		}

		void remote_kexinit(const Buffer& kexinit)
		{
			if (role_ == ClientRole)
				server_kexinit_ = kexinit;
			else
				client_kexinit_ = kexinit;
		}

		void activate_chosen(void)
		{
			active_algorithms_ = chosen_algorithms_;

			client_to_server_iv_ = generate_key("A", active_algorithms_.client_to_server_.encryption_->iv_size());
			server_to_client_iv_ = generate_key("B", active_algorithms_.server_to_client_.encryption_->iv_size());
			client_to_server_key_ = generate_key("C", active_algorithms_.client_to_server_.encryption_->key_size());
			server_to_client_key_ = generate_key("D", active_algorithms_.server_to_client_.encryption_->key_size());
			client_to_server_integrity_key_ = generate_key("E", active_algorithms_.client_to_server_.mac_->key_size());
			server_to_client_integrity_key_ = generate_key("F", active_algorithms_.server_to_client_.mac_->key_size());
		}

		Buffer generate_key(const std::string& x, unsigned key_size)
		{
			Buffer key;
			while (key.length() < key_size) {
				Buffer input;
				input.append(shared_secret_);
				input.append(exchange_hash_);
				if (key.empty()) {
					input.append(x);
					input.append(session_id_);
				} else {
					input.append(key);
				}
				if (!active_algorithms_.key_exchange_->hash(&key, &input))
					HALT("/ssh/session") << "Hash failed in generating key.";
			}
			if (key.length() > key_size)
				key.truncate(key_size);
			return (key);
		}
	};
}

#endif /* !SSH_SSH_SESSION_H */
