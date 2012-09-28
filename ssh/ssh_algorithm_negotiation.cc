#include <map>

#include <common/buffer.h>
#include <common/endian.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_session.h>

namespace {
	static uint8_t constant_cookie[16] = {
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xa0, 0xb0,
		0xc0, 0xd0, 0xe0, 0xf0
	};

	template<typename T>
	std::vector<Buffer> names(T list)
	{
		typename T::const_iterator it;
		std::vector<Buffer> vec;

		for (it = list.begin(); it != list.end(); ++it)
			vec.push_back(it->first);
		return (vec);
	}

	template<typename T>
	bool choose_algorithm(SSH::Role role,
			      std::map<std::string, T>& chosen_map,
			      std::map<std::string, T>& algorithm_map,
			      Buffer *in, const std::string& type)
	{
		std::vector<Buffer> local_algorithms = names(algorithm_map);
		std::vector<Buffer> remote_algorithms;
		if (!SSH::NameList::decode(remote_algorithms, in)) {
			ERROR("/ssh/algorithm/negotiation") << "Failed to decode " << type << " name-list.";
			return (false);
		}

		if (remote_algorithms.empty() && local_algorithms.empty()) {
			DEBUG("/ssh/algorithm/negotiation") << "Neither client nor server has any preference in " << type << " algorithms.";
			return (true);
		}

		const std::vector<Buffer> *client_algorithms;
		const std::vector<Buffer> *server_algorithms;
		if (role == SSH::ClientRole) {
			client_algorithms = &local_algorithms;
			server_algorithms = &remote_algorithms;
		} else {
			client_algorithms = &remote_algorithms;
			server_algorithms = &local_algorithms;
		}

		std::vector<Buffer>::const_iterator it;
		for (it = client_algorithms->begin();
		     it != client_algorithms->end(); ++it) {
			std::vector<Buffer>::const_iterator it2;

			for (it2 = server_algorithms->begin();
			     it2 != server_algorithms->end(); ++it2) {
				const Buffer& server = *it2;
				if (!it->equal(&server))
					continue;
				std::string algorithm;
				it->extract(algorithm);
				chosen_map[algorithm] = algorithm_map[algorithm];

				DEBUG("/ssh/algorithm/negotiation") << "Selected " << type << " algorithm " << algorithm;
				return (true);
			}
		}

		ERROR("/ssh/algorithm/negotiation") << "Failed to choose " << type << " algorithm.";
		return (false);
	}
}

bool
SSH::AlgorithmNegotiation::input(Buffer *in)
{
	switch (in->peek()) {
	case SSH::Message::KeyExchangeInitializationMessage:
		session_->remote_kexinit(*in);
		if (!choose_algorithms(in)) {
			ERROR(log_) << "Unable to negotiate algorithms.";
			return (false);
		}
		DEBUG(log_) << "Chose algorithms.";
		return (true);
	default:
		DEBUG(log_) << "Unsupported algorithm negotiation message:" << std::endl << in->hexdump();
		return (false);
	}
}

bool
SSH::AlgorithmNegotiation::output(Buffer *out)
{
	ASSERT(log_, out->empty());

	out->append(SSH::Message::KeyExchangeInitializationMessage);
	out->append(constant_cookie, sizeof constant_cookie); /* XXX Synchronous random byte generation?  */
	SSH::NameList::encode(out, names(algorithms_.key_exchange_map_));
	SSH::NameList::encode(out, names(algorithms_.server_host_key_map_));
	SSH::NameList::encode(out, names(algorithms_.encryption_client_to_server_map_));
	SSH::NameList::encode(out, names(algorithms_.encryption_server_to_client_map_));
	SSH::NameList::encode(out, names(algorithms_.mac_client_to_server_map_));
	SSH::NameList::encode(out, names(algorithms_.mac_server_to_client_map_));
	SSH::NameList::encode(out, names(algorithms_.compression_client_to_server_map_));
	SSH::NameList::encode(out, names(algorithms_.compression_server_to_client_map_));
	SSH::NameList::encode(out, names(algorithms_.language_client_to_server_map_));
	SSH::NameList::encode(out, names(algorithms_.language_server_to_client_map_));
	out->append(SSH::Boolean::False);
	uint32_t reserved(0);
	out->append(&reserved);

	session_->local_kexinit(*out);

	return (true);
}

bool
SSH::AlgorithmNegotiation::choose_algorithms(Buffer *in)
{
	in->skip(17);

	chosen_ = Algorithms();
	if (!choose_algorithm(session_->role_, chosen_.key_exchange_map_, algorithms_.key_exchange_map_, in, "Key Exchange"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.server_host_key_map_, algorithms_.server_host_key_map_, in, "Server Host Key"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.encryption_client_to_server_map_, algorithms_.encryption_client_to_server_map_, in, "Encryption (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.encryption_server_to_client_map_, algorithms_.encryption_server_to_client_map_, in, "Encryption (Server->Client)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.mac_client_to_server_map_, algorithms_.mac_client_to_server_map_, in, "MAC (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.mac_server_to_client_map_, algorithms_.mac_server_to_client_map_, in, "MAC (Server->Client)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.compression_client_to_server_map_, algorithms_.compression_client_to_server_map_, in, "Compression (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.compression_server_to_client_map_, algorithms_.compression_server_to_client_map_, in, "Compression (Server->Client)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.language_client_to_server_map_, algorithms_.language_client_to_server_map_, in, "Language (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, chosen_.language_server_to_client_map_, algorithms_.language_server_to_client_map_, in, "Language (Server->Client)"))
		return (false);

	return (true);
}
