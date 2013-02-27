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

#ifndef	SSH_SSH_SESSION_H
#define	SSH_SSH_SESSION_H

namespace SSH {
	class AlgorithmNegotiation;
	class Compression;
	class Encryption;
	class KeyExchange;
	class Language;
	class MAC;
	class ServerHostKey;

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
	private:
		Buffer client_to_server_iv_;	/* Initial client-to-server IV.  */
		Buffer server_to_client_iv_;	/* Initial server-to-client IV.  */
		Buffer client_to_server_key_;	/* Client-to-server encryption key.  */
		Buffer server_to_client_key_;	/* Server-to-client encryption key.  */
		Buffer client_to_server_integrity_key_;	/* Client-to-server integrity key.  */
		Buffer server_to_client_integrity_key_;	/* Server-to-client integrity key.  */
	public:
		uint32_t local_sequence_number_;	/* Our packet sequence number.  */
		uint32_t remote_sequence_number_;	/* Our peer's packet sequence number.  */

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
		  server_to_client_integrity_key_(),
		  local_sequence_number_(0),
		  remote_sequence_number_(0)
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

		void activate_chosen(void);
	private:
		Buffer generate_key(const std::string& x, unsigned key_size);
	};
}

#endif /* !SSH_SSH_SESSION_H */
