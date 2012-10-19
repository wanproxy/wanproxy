#include <common/buffer.h>
#include <common/endian.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_language.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_server_host_key.h>
#include <ssh/ssh_session.h>
#include <ssh/ssh_transport_pipe.h>

namespace {
	static uint8_t constant_cookie[16] = {
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xa0, 0xb0,
		0xc0, 0xd0, 0xe0, 0xf0
	};

	template<typename T>
	std::vector<Buffer> names(const std::list<T>& list)
	{
		typename std::list<T>::const_iterator it;
		std::vector<Buffer> vec;

		for (it = list.begin(); it != list.end(); ++it) {
			const T alg = *it;
			vec.push_back(alg->name());
		}
		return (vec);
	}

	template<typename T>
	bool choose_algorithm(SSH::Role role,
			      T *chosenp,
			      std::list<T>& algorithm_list,
			      Buffer *in, const std::string& type)
	{
		std::vector<Buffer> local_algorithms = names(algorithm_list);
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

				typename std::list<T>::const_iterator ait;
				for (ait = algorithm_list.begin(); ait != algorithm_list.end(); ++ait) {
					const T alg = *ait;
					if (alg->name() != algorithm)
						continue;

					*chosenp = alg->clone();

					DEBUG("/ssh/algorithm/negotiation") << "Selected " << type << " algorithm " << algorithm;
					return (true);
				}
				NOTREACHED("/ssh/algorithm/negotiation");
			}
		}

		ERROR("/ssh/algorithm/negotiation") << "Failed to choose " << type << " algorithm.";
		return (false);
	}
}

void
SSH::AlgorithmNegotiation::add_algorithms(void)
{
	SSH::KeyExchange::add_algorithms(session_);
	if (session_->role_ == ClientRole)
		SSH::ServerHostKey::add_client_algorithms(session_);
	SSH::Encryption::add_algorithms(session_);
	SSH::MAC::add_algorithms(session_);
	add_algorithm(SSH::Compression::none());
	/* XXX Add languages?  */
}

bool
SSH::AlgorithmNegotiation::input(SSH::TransportPipe *pipe, Buffer *in)
{
	Buffer packet;

	switch (in->peek()) {
	case SSH::Message::KeyExchangeInitializationMessage:
		session_->remote_kexinit(*in);
		if (!choose_algorithms(in)) {
			ERROR(log_) << "Unable to negotiate algorithms.";
			return (false);
		}
		DEBUG(log_) << "Chose algorithms.";

		if (session_->role_ == ClientRole && session_->chosen_algorithms_.key_exchange_ != NULL) {
			Buffer out;
			if (!session_->chosen_algorithms_.key_exchange_->init(&out)) {
				ERROR(log_) << "Could not start new key exchange.";
				return (false);
			}
			if (!out.empty())
				pipe->send(&out);
		}
		return (true);
	case SSH::Message::NewKeysMessage:
		packet.append(SSH::Message::NewKeysMessage);
		pipe->send(&packet);

		session_->activate_chosen();
		DEBUG(log_) << "Switched to new keys.";
		return (true);
	default:
		DEBUG(log_) << "Unsupported algorithm negotiation message:" << std::endl << in->hexdump();
		return (false);
	}
}

bool
SSH::AlgorithmNegotiation::init(Buffer *out)
{
	ASSERT(log_, out->empty());

	out->append(SSH::Message::KeyExchangeInitializationMessage);
	out->append(constant_cookie, sizeof constant_cookie); /* XXX Synchronous random byte generation?  */
	SSH::NameList::encode(out, names(algorithms_.key_exchange_list_));
	SSH::NameList::encode(out, names(algorithms_.server_host_key_list_));
	SSH::NameList::encode(out, names(algorithms_.encryption_client_to_server_list_));
	SSH::NameList::encode(out, names(algorithms_.encryption_server_to_client_list_));
	SSH::NameList::encode(out, names(algorithms_.mac_client_to_server_list_));
	SSH::NameList::encode(out, names(algorithms_.mac_server_to_client_list_));
	SSH::NameList::encode(out, names(algorithms_.compression_client_to_server_list_));
	SSH::NameList::encode(out, names(algorithms_.compression_server_to_client_list_));
	SSH::NameList::encode(out, names(algorithms_.language_client_to_server_list_));
	SSH::NameList::encode(out, names(algorithms_.language_server_to_client_list_));
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

	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.key_exchange_, algorithms_.key_exchange_list_, in, "Key Exchange"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.server_host_key_, algorithms_.server_host_key_list_, in, "Server Host Key"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.client_to_server_.encryption_, algorithms_.encryption_client_to_server_list_, in, "Encryption (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.server_to_client_.encryption_, algorithms_.encryption_server_to_client_list_, in, "Encryption (Server->Client)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.client_to_server_.mac_, algorithms_.mac_client_to_server_list_, in, "MAC (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.server_to_client_.mac_, algorithms_.mac_server_to_client_list_, in, "MAC (Server->Client)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.client_to_server_.compression_, algorithms_.compression_client_to_server_list_, in, "Compression (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.server_to_client_.compression_, algorithms_.compression_server_to_client_list_, in, "Compression (Server->Client)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.client_to_server_.language_, algorithms_.language_client_to_server_list_, in, "Language (Client->Server)"))
		return (false);
	if (!choose_algorithm(session_->role_, &session_->chosen_algorithms_.server_to_client_.language_, algorithms_.language_server_to_client_list_, in, "Language (Server->Client)"))
		return (false);

	return (true);
}
