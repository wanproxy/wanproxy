#ifndef	SSH_SSH_ALGORITHM_NEGOTIATION_H
#define	SSH_SSH_ALGORITHM_NEGOTIATION_H

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

			Algorithms(std::map<std::string, KeyExchange *> key_exchange_list,
				   std::map<std::string, ServerHostKey *> server_host_key_list,
				   std::map<std::string, Encryption *> encryption_client_to_server_list,
				   std::map<std::string, Encryption *> encryption_server_to_client_list,
				   std::map<std::string, MAC *> mac_client_to_server_list,
				   std::map<std::string, MAC *> mac_server_to_client_list,
				   std::map<std::string, Compression *> compression_client_to_server_list,
				   std::map<std::string, Compression *> compression_server_to_client_list,
				   std::map<std::string, Language *> language_client_to_server_list,
				   std::map<std::string, Language *> language_server_to_client_list)
			: key_exchange_map_(key_exchange_list),
			  server_host_key_map_(server_host_key_list),
			  encryption_client_to_server_map_(encryption_client_to_server_list),
			  encryption_server_to_client_map_(encryption_server_to_client_list),
			  mac_client_to_server_map_(mac_client_to_server_list),
			  mac_server_to_client_map_(mac_server_to_client_list),
			  compression_client_to_server_map_(compression_client_to_server_list),
			  compression_server_to_client_map_(compression_server_to_client_list),
			  language_client_to_server_map_(language_client_to_server_list),
			  language_server_to_client_map_(language_server_to_client_list)
			{ }

		};

		LogHandle log_;
		Role role_;
		Algorithms algorithms_;
		Algorithms chosen_;
		Algorithms active_;
		Buffer sent_initialization_;
		Buffer received_initialization_;
		Buffer active_initialization_;
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
		  algorithms_(list_to_map(key_exchange_list),
			      list_to_map(server_host_key_list),
			      list_to_map(encryption_client_to_server_list),
			      list_to_map(encryption_server_to_client_list),
			      list_to_map(mac_client_to_server_list),
			      list_to_map(mac_server_to_client_list),
			      list_to_map(compression_client_to_server_list),
			      list_to_map(compression_server_to_client_list),
			      list_to_map(language_client_to_server_list),
			      list_to_map(language_server_to_client_list)),
		  chosen_(),
		  active_(),
		  sent_initialization_(),
		  received_initialization_()
		{ }

		AlgorithmNegotiation(Role role, std::vector<KeyExchange *> key_exchange_list,
				     std::vector<ServerHostKey *> server_host_key_list,
				     std::vector<Encryption *> encryption_list,
				     std::vector<MAC *> mac_list,
				     std::vector<Compression *> compression_list,
				     std::vector<Language *> language_list)
		: log_("/ssh/algorithm/negotiation"),
		  role_(role),
		  algorithms_(list_to_map(key_exchange_list),
			      list_to_map(server_host_key_list),
			      list_to_map(encryption_list), list_to_map(encryption_list),
			      list_to_map(mac_list), list_to_map(mac_list),
			      list_to_map(compression_list), list_to_map(compression_list),
			      list_to_map(language_list), list_to_map(language_list)),
		  chosen_(),
		  active_(),
		  sent_initialization_(),
		  received_initialization_()
		{ }

		AlgorithmNegotiation(Role role, KeyExchange *key_exchange,
				     ServerHostKey *server_host_key,
				     Encryption *encryption, MAC *mac,
				     Compression *compression,
				     Language *language)
		: log_("/ssh/algorithm/negotiation"),
		  role_(role),
		  algorithms_(),
		  chosen_(),
		  active_(),
		  sent_initialization_(),
		  received_initialization_()
		{
			if (key_exchange != NULL)
				algorithms_.key_exchange_map_[key_exchange->name()] = key_exchange;
			if (server_host_key != NULL)
				algorithms_.server_host_key_map_[server_host_key->name()] = server_host_key;
			if (encryption != NULL)
				algorithms_.encryption_client_to_server_map_[encryption->name()] = encryption;
			algorithms_.encryption_server_to_client_map_ = algorithms_.encryption_client_to_server_map_;
			if (mac != NULL)
				algorithms_.mac_client_to_server_map_[mac->name()] = mac;
			algorithms_.mac_server_to_client_map_ = algorithms_.mac_client_to_server_map_;
			if (compression != NULL)
				algorithms_.compression_client_to_server_map_[compression->name()] = compression;
			algorithms_.compression_server_to_client_map_ = algorithms_.compression_client_to_server_map_;
			if (language != NULL)
				algorithms_.language_client_to_server_map_[language->name()] = language;
			algorithms_.language_server_to_client_map_ = algorithms_.language_client_to_server_map_;
		}

		/* XXX Add a variant that takes only server_host_key_list and fills in suitable defaults.  */

		~AlgorithmNegotiation()
		{
			(void)role_; /* XXX role_ is not yet used, but may be soon.  */
		}

		bool input(Buffer *);
		bool output(Buffer *);

		SSH::KeyExchange *key_exchange_algorithm(void) const
		{
			if (chosen_.key_exchange_map_.empty())
				return (NULL);
			ASSERT(log_, chosen_.key_exchange_map_.size() == 1);
			return (chosen_.key_exchange_map_.begin()->second);
		}

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
