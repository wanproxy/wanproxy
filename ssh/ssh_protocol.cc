/*
 * Copyright (c) 2012-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
	BigEndian::extract(&len, in);
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
	BigEndian::append(out, in);
}

bool
SSH::UInt32::decode(uint32_t *outp, Buffer *in)
{
	if (in->length() < sizeof *outp)
		return (false);
	BigEndian::extract(outp, in);
	in->skip(sizeof *outp);
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
	BigEndian::extract(&len, in);
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
