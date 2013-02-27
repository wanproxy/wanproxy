/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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

/*
 * The two variants are so similar it seems most reasonable to support both in
 * one file like this.
 */
#if defined(__FreeBSD__)
#else
#define USE_LIBUUID
#endif

#ifdef USE_LIBUUID
#include <uuid/uuid.h>
#else
#include <uuid.h>
#endif

#include <common/buffer.h>
#include <common/uuid/uuid.h>

bool
UUID::decode(Buffer *buf)
{
	if (buf->length() < UUID_SIZE)
		return (false);

	uint8_t str[UUID_SIZE];
	buf->moveout(str, sizeof str);
	string_ = std::string((const char *)str, sizeof str);

	uuid_t uuid;
#ifdef USE_LIBUUID
	int rv = uuid_parse(string_.c_str(), uuid);
	if (rv == -1)
		return (false);
	ASSERT("/uuid/libuuid", rv == 0);
#else
	uint32_t status;
	uuid_from_string(string_.c_str(), &uuid, &status);
	if (status != uuid_s_ok)
		return (false);
#endif

	return (true);
}

bool
UUID::encode(Buffer *buf) const
{
	ASSERT("/uuid", string_.length() == UUID_SIZE);
	buf->append(string_);

	return (true);
}

void
UUID::generate(void)
{
	uuid_t uuid;

#ifdef USE_LIBUUID
	char str[UUID_SIZE + 1];

	uuid_generate(uuid);
	uuid_unparse(uuid, str);
	string_ = str;
#else
	char *p;

	uuid_create(&uuid, NULL);
	uuid_to_string(&uuid, &p, NULL);
	ASSERT("/uuid/libc", p != NULL);
	string_ = p;
	free(p);
#endif
	ASSERT("/uuid", string_.length() == UUID_SIZE);
}
