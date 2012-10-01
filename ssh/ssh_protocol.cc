#include <openssl/bn.h>

#include <common/buffer.h>
#include <common/endian.h>

#include <ssh/ssh_protocol.h>

void
SSH::String::encode(Buffer *out, const Buffer& in)
{
	SSH::UInt32::encode(out, in.length());
	if (!in.empty())
		out->append(in);
}

void
SSH::String::encode(Buffer *out, Buffer *in)
{
	SSH::String::encode(out, *in);
	if (!in->empty())
		in->clear();
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
SSH::UInt32::encode(Buffer *out, uint32_t in)
{
	in = BigEndian::encode(in);
	out->append(&in);
}

bool
SSH::UInt32::decode(uint32_t *outp, Buffer *in)
{
	uint32_t out;

	if (in->length() < sizeof *outp)
		return (false);
	in->extract(&out);
	*outp = BigEndian::decode(out);
	return (true);
}

void
SSH::MPInt::encode(Buffer *out, const BIGNUM *in)
{
	if (BN_is_negative(in)) {
		HALT("/ssh/mpint/encode") << "Negative numbers not yet implemented.";
	}
	uint8_t buf[BN_num_bytes(in)];
	BN_bn2bin(in, buf);
	if ((buf[0] & 0x80) == 0x80) {
		SSH::UInt32::encode(out, sizeof buf + 1);
		out->append((uint8_t)0x00);
		out->append(buf, sizeof buf);
	} else {
		SSH::UInt32::encode(out, sizeof buf);
		out->append(buf, sizeof buf);
	}
}

bool
SSH::MPInt::decode(BIGNUM **outp, Buffer *in)
{
	uint32_t len;
	BIGNUM *out;

	if (in->length() < sizeof len)
		return (false);
	in->extract(&len);
	len = BigEndian::decode(len);

	if (len == 0) {
		out = BN_new();
		if (out == NULL)
			return (false);
		in->skip(sizeof len);
		BN_zero(out);
		*outp = out;
		return (true);
	}

	if (in->length() < sizeof len + len)
		return (false);

	uint8_t buf[len];
	in->copyout(buf, sizeof len, len);
	out = BN_bin2bn(buf, sizeof buf, NULL);
	if (out == NULL)
		return (false);
	in->skip(sizeof len + len);
	*outp = out;
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
