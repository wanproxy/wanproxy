#ifndef	SSH_ALGORITHM_NEGOTIATION_H
#define	SSH_ALGORITHM_NEGOTIATION_H

#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_language.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_server_host_key.h>

class SSHAlgorithmNegotiation {
	std::map<std::string, SSHKeyExchange *> key_exchange_map_;
	std::map<std::string, SSHServerHostKey *> server_host_key_map_;
	std::map<std::string, SSHEncryption *> encryption_client_to_server_map_;
	std::map<std::string, SSHEncryption *> encryption_server_to_client_map_;
	std::map<std::string, SSHMAC *> mac_client_to_server_map_;
	std::map<std::string, SSHMAC *> mac_server_to_client_map_;
	std::map<std::string, SSHCompression *> compression_client_to_server_map_;
	std::map<std::string, SSHCompression *> compression_server_to_client_map_;
	std::map<std::string, SSHLanguage *> language_client_to_server_map_;
	std::map<std::string, SSHLanguage *> language_server_to_client_map_;
public:
	SSHAlgorithmNegotiation(std::vector<SSHKeyExchange *> key_exchange_list,
				std::vector<SSHServerHostKey *> server_host_key_list,
				std::vector<SSHEncryption *> encryption_client_to_server_list,
				std::vector<SSHEncryption *> encryption_server_to_client_list,
				std::vector<SSHMAC *> mac_client_to_server_list,
				std::vector<SSHMAC *> mac_server_to_client_list,
				std::vector<SSHCompression *> compression_client_to_server_list,
				std::vector<SSHCompression *> compression_server_to_client_list,
				std::vector<SSHLanguage *> language_client_to_server_list,
				std::vector<SSHLanguage *> language_server_to_client_list)
	: key_exchange_map_(list_to_map(key_exchange_list)),
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

	SSHAlgorithmNegotiation(std::vector<SSHKeyExchange *> key_exchange_list,
				std::vector<SSHServerHostKey *> server_host_key_list,
				std::vector<SSHEncryption *> encryption_list,
				std::vector<SSHMAC *> mac_list,
				std::vector<SSHCompression *> compression_list,
				std::vector<SSHLanguage *> language_list)
	: key_exchange_map_(list_to_map(key_exchange_list)),
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

	/* XXX Add a variant that takes only server_host_key_list and fills in suitable defaults.  */

	~SSHAlgorithmNegotiation()
	{ }

	bool input(Buffer *)
	{
		return (false);
	}

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

#endif /* !SSH_ALGORITHM_NEGOTIATION_H */
