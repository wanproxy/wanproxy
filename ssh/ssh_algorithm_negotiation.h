/*
 * Copyright (c) 2011-2013 Juli Mallett. All rights reserved.
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

#ifndef	SSH_SSH_ALGORITHM_NEGOTIATION_H
#define	SSH_SSH_ALGORITHM_NEGOTIATION_H

#include <list>

namespace SSH {
	class Compression;
	class Encryption;
	class KeyExchange;
	class Language;
	class MAC;
	class ServerHostKey;
	struct Session;
	class TransportPipe;

	class AlgorithmNegotiation {
		struct Algorithms {
			std::list<KeyExchange *> key_exchange_list_;
			std::list<ServerHostKey *> server_host_key_list_;
			std::list<Encryption *> encryption_client_to_server_list_;
			std::list<Encryption *> encryption_server_to_client_list_;
			std::list<MAC *> mac_client_to_server_list_;
			std::list<MAC *> mac_server_to_client_list_;
			std::list<Compression *> compression_client_to_server_list_;
			std::list<Compression *> compression_server_to_client_list_;
			std::list<Language *> language_client_to_server_list_;
			std::list<Language *> language_server_to_client_list_;

			Algorithms(void)
			: key_exchange_list_(),
			  server_host_key_list_(),
			  encryption_client_to_server_list_(),
			  encryption_server_to_client_list_(),
			  mac_client_to_server_list_(),
			  mac_server_to_client_list_(),
			  compression_client_to_server_list_(),
			  compression_server_to_client_list_(),
			  language_client_to_server_list_(),
			  language_server_to_client_list_()
			{ }
		};

		LogHandle log_;
		Session *session_;
		Algorithms algorithms_;
	public:
		AlgorithmNegotiation(Session *session)
		: log_("/ssh/algorithm/negotiation"),
		  session_(session),
		  algorithms_()
		{ }

		/* XXX Add a variant that takes only server_host_key_list and fills in suitable defaults.  */

		~AlgorithmNegotiation()
		{ }

		void add_algorithm(KeyExchange *key_exchange)
		{
			algorithms_.key_exchange_list_.push_back(key_exchange);
		}

		void add_algorithm(ServerHostKey *server_host_key)
		{
			algorithms_.server_host_key_list_.push_back(server_host_key);
		}

		void add_algorithm(Encryption *encryption)
		{
			algorithms_.encryption_client_to_server_list_.push_back(encryption);
			algorithms_.encryption_server_to_client_list_.push_back(encryption);
		}

		void add_algorithm(MAC *mac)
		{
			algorithms_.mac_client_to_server_list_.push_back(mac);
			algorithms_.mac_server_to_client_list_.push_back(mac);
		}

		void add_algorithm(Compression *compression)
		{
			algorithms_.compression_client_to_server_list_.push_back(compression);
			algorithms_.compression_server_to_client_list_.push_back(compression);
		}

		void add_algorithm(Language *language)
		{
			algorithms_.language_client_to_server_list_.push_back(language);
			algorithms_.language_server_to_client_list_.push_back(language);
		}

		void add_algorithms(void);

		bool input(TransportPipe *, Buffer *);
		bool init(Buffer *);

	private:
		bool choose_algorithms(Buffer *);
	};
}

#endif /* !SSH_SSH_ALGORITHM_NEGOTIATION_H */
