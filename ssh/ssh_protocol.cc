#include <common/buffer.h>
#include <common/endian.h>

#include <ssh/ssh_protocol.h>

void
SSH::String::encode(Buffer *out, Buffer *in)
{
	uint32_t len;

	if (in->empty()) {
		len = 0;
		out->append(&len);
		return;
	}

	len = in->length();
	len = BigEndian::encode(len);

	out->append(&len);
	in->moveout(out);
}

bool
SSH::String::decode(Buffer *out, Buffer *in)
{
	uint32_t len;

	if (in->length() < sizeof len)
		return (false);
	in->extract(&len);
	len = BigEndian::decode(len);

	if (len == 0) {
		in->skip(sizeof len);
		return (true);
	}

	if (in->length() < sizeof len + len)
		return (false);

	in->moveout(out, sizeof len, len);
	return (true);
}

void
SSH::NameList::encode(Buffer *out, const std::vector<Buffer>& in)
{
	Buffer merged;

	merged = Buffer::join(in, ",");
	SSH::String::encode(out, &merged);
}

bool
SSH::NameList::decode(std::vector<Buffer>& out, Buffer *in)
{
	Buffer merged;

	if (!SSH::String::decode(&merged, in))
		return (false);

	out = merged.split(',');
	return (true);
}
