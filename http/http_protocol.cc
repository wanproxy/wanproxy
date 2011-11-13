#include <common/buffer.h>

#include <http/http_protocol.h>

namespace {
	static bool decode_nibble(uint8_t *out, uint8_t ch)
	{
		if (ch >= '0' && ch <= '9') {
			*out |= 0x00 + (ch - '0');
			return (true);
		}
		if (ch >= 'a' && ch <= 'f') {
			*out |= 0x0a + (ch - 'a');
			return (true);
		}
		if (ch >= 'A' && ch <= 'F') {
			*out |= 0x0a + (ch - 'A');
			return (true);
		}
		return (false);
	}
}

bool
HTTPProtocol::DecodeURI(Buffer *encoded, Buffer *decoded)
{
	if (encoded->empty())
		return (true);

	for (;;) {
		unsigned pos;
		if (!encoded->find('%', &pos)) {
			decoded->append(encoded);
			encoded->clear();
			return (true);
		}
		if (pos != 0)
			encoded->moveout(decoded, pos);
		if (encoded->length() < 3)
			return (false);
		uint8_t vis[2];
		encoded->copyout(vis, 1, 2);
		uint8_t byte = 0x00;
		if (!decode_nibble(&byte, vis[0]))
			return (false);
		byte <<= 4;
		if (!decode_nibble(&byte, vis[1]))
			return (false);
		decoded->append(byte);
		encoded->skip(3);
	}
}
