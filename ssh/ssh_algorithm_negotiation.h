#ifndef	SSH_SSH_ALGORITHM_NEGOTIATION_H
#define	SSH_SSH_ALGORITHM_NEGOTIATION_H

#include <map>

#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_language.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_server_host_key.h>

namespace SSH {
	struct Session;
	class TransportPipe;

	class AlgorithmNegotiation {
		struct Algorithms {
			std::map<std::string, KeyExchange *> key_exchange_map_;
			std::map<std::string, ServerHostKey *> server_host_key_map_;
			std::map<std::string, Encryption *> encryption_client_to_server_map_;
			std::map<std::string, Encryption *> encryption_server_to_client_map_;
			std::map<std::string, MAC *> mac_client_to_server_map_;
			std::map<std::string, MAC *> mac_server_to_client_map_;
			std::map<std::string, Compression *> compression_client_to_server_map_;
			std::map<std::string, Compression *> compression_server_to_client_map_;
			std::map<std::string, Language *> language_client_to_server_map_;
			std::map<std::string, Language *> language_server_to_client_map_;

			Algorithms(void)
			: key_exchange_map_(),
			  server_host_key_map_(),
			  encryption_client_to_server_map_(),
			  encryption_server_to_client_map_(),
			  mac_client_to_server_map_(),
			  mac_server_to_client_map_(),
			  compression_client_to_server_map_(),
			  compression_server_to_client_map_(),
			  language_client_to_server_map_(),
			  language_server_to_client_map_()
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
			algorithms_.key_exchange_map_[key_exchange->name()] = key_exchange;
		}

		void add_algorithm(ServerHostKey *server_host_key)
		{
			algorithms_.server_host_key_map_[server_host_key->name()] = server_host_key;
		}

		void add_algorithm(Encryption *encryption)
		{
			algorithms_.encryption_client_to_server_map_[encryption->name()] = encryption;
			algorithms_.encryption_server_to_client_map_[encryption->name()] = encryption;
		}

		void add_algorithm(MAC *mac)
		{
			algorithms_.mac_client_to_server_map_[mac->name()] = mac;
			algorithms_.mac_server_to_client_map_[mac->name()] = mac;
		}

		void add_algorithm(Compression *compression)
		{
			algorithms_.compression_client_to_server_map_[compression->name()] = compression;
			algorithms_.compression_server_to_client_map_[compression->name()] = compression;
		}

		void add_algorithm(Language *language)
		{
			algorithms_.language_client_to_server_map_[language->name()] = language;
			algorithms_.language_server_to_client_map_[language->name()] = language;
		}

		void add_algorithms(void);

		bool input(TransportPipe *, Buffer *);
		bool init(Buffer *);

	private:
		bool choose_algorithms(Buffer *);

		template<typename T>
		static std::map<std::string, typename T::value_type> list_to_map(const T& list)
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
}

#endif /* !SSH_SSH_ALGORITHM_NEGOTIATION_H */
