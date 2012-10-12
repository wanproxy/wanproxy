#ifndef	SSH_SSH_ALGORITHM_NEGOTIATION_H
#define	SSH_SSH_ALGORITHM_NEGOTIATION_H

#include <list>

#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_language.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_server_host_key.h>

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
