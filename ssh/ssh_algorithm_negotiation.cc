#include <map>

#include <common/buffer.h>
#include <common/endian.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_protocol.h>

namespace {
	static uint8_t constant_cookie[16] = {
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xa0, 0xb0,
		0xc0, 0xd0, 0xe0, 0xf0
	};

	template<typename T>
	Buffer name_list(T list)
	{
		typename T::const_iterator it;
		std::vector<Buffer> names;
		Buffer merged;
		uint32_t len;
		Buffer out;

		for (it = list.begin(); it != list.end(); ++it)
			names.push_back(it->first);
		merged = Buffer::join(names, ",");
		len = merged.length();
		len = BigEndian::encode(len);
		out.append(&len);
		out.append(merged);
		return (out);
	}
}

bool
SSH::AlgorithmNegotiation::input(Buffer *in)
{
	switch (in->peek()) {
	case SSH::Message::KeyExchangeInitializationMessage:
		DEBUG(log_) << "Just ignoring initialization message.";
		return (true);
	default:
		DEBUG(log_) << "Unsupported algorithm negotiation message:" << std::endl << in->hexdump();
		return (false);
	}
}

bool
SSH::AlgorithmNegotiation::output(Buffer *out)
{
	ASSERT(out->empty());

	out->append(SSH::Message::KeyExchangeInitializationMessage);
	out->append(constant_cookie, sizeof constant_cookie); /* XXX Synchronous random byte generation?  */
	out->append(name_list(key_exchange_map_));
	out->append(name_list(server_host_key_map_));
	out->append(name_list(encryption_client_to_server_map_));
	out->append(name_list(encryption_server_to_client_map_));
	out->append(name_list(mac_client_to_server_map_));
	out->append(name_list(mac_server_to_client_map_));
	out->append(name_list(compression_client_to_server_map_));
	out->append(name_list(compression_server_to_client_map_));
	out->append(name_list(language_client_to_server_map_));
	out->append(name_list(language_server_to_client_map_));
	out->append(SSH::Boolean::False);
	uint32_t reserved(0);
	out->append(&reserved);

	return (true);
}
