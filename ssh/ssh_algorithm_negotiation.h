#ifndef	SSH_ALGORITHM_NEGOTIATION_H
#define	SSH_ALGORITHM_NEGOTIATION_H

#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_language.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_server_host_key.h>

namespace SSH {
	class AlgorithmNegotiation {
	public:
		enum Role {
			ClientRole,
			ServerRole,
		};
	private:
		LogHandle log_;
		Role role_;
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
	public:
		AlgorithmNegotiation(Role role, std::vector<KeyExchange *> key_exchange_list,
				     std::vector<ServerHostKey *> server_host_key_list,
				     std::vector<Encryption *> encryption_client_to_server_list,
				     std::vector<Encryption *> encryption_server_to_client_list,
				     std::vector<MAC *> mac_client_to_server_list,
				     std::vector<MAC *> mac_server_to_client_list,
				     std::vector<Compression *> compression_client_to_server_list,
				     std::vector<Compression *> compression_server_to_client_list,
				     std::vector<Language *> language_client_to_server_list,
				     std::vector<Language *> language_server_to_client_list)
		: log_("/ssh/algorithm/negotiation"),
		  role_(role),
		  key_exchange_map_(list_to_map(key_exchange_list)),
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

		AlgorithmNegotiation(Role role, std::vector<KeyExchange *> key_exchange_list,
				     std::vector<ServerHostKey *> server_host_key_list,
				     std::vector<Encryption *> encryption_list,
				     std::vector<MAC *> mac_list,
				     std::vector<Compression *> compression_list,
				     std::vector<Language *> language_list)
		: log_("/ssh/algorithm/negotiation"),
		  role_(role),
		  key_exchange_map_(list_to_map(key_exchange_list)),
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

		AlgorithmNegotiation(Role role, KeyExchange *key_exchange,
				     ServerHostKey *server_host_key,
				     Encryption *encryption, MAC *mac,
				     Compression *compression,
				     Language *language)
		: log_("/ssh/algorithm/negotiation"),
		  role_(role),
		  key_exchange_map_(),
		  server_host_key_map_(),
		  encryption_client_to_server_map_(),
		  encryption_server_to_client_map_(),
		  mac_client_to_server_map_(),
		  mac_server_to_client_map_(),
		  compression_client_to_server_map_(),
		  compression_server_to_client_map_(),
		  language_client_to_server_map_(),
		  language_server_to_client_map_()
		{
			if (key_exchange != NULL)
				key_exchange_map_[key_exchange->name()] = key_exchange;
			if (server_host_key != NULL)
				server_host_key_map_[server_host_key->name()] = server_host_key;
			if (encryption != NULL)
				encryption_client_to_server_map_[encryption->name()] = encryption;
			encryption_server_to_client_map_ = encryption_client_to_server_map_;
			if (mac != NULL)
				mac_client_to_server_map_[mac->name()] = mac;
			mac_server_to_client_map_ = mac_client_to_server_map_;
			if (compression != NULL)
				compression_client_to_server_map_[compression->name()] = compression;
			compression_server_to_client_map_ = compression_client_to_server_map_;
			if (language != NULL)
				language_client_to_server_map_[language->name()] = language;
			language_server_to_client_map_ = language_client_to_server_map_;
		}

		/* XXX Add a variant that takes only server_host_key_list and fills in suitable defaults.  */

		~AlgorithmNegotiation()
		{ }

		bool input(Buffer *);
		bool output(Buffer *);

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
}

#endif /* !SSH_ALGORITHM_NEGOTIATION_H */
