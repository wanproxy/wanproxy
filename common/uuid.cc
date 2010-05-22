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
#include <common/uuid.h>

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
	ASSERT(rv == 0);
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
	ASSERT(string_.length() == UUID_SIZE);
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
	ASSERT(p != NULL);
	string_ = p;
	free(p);
#endif
	ASSERT(string_.length() == UUID_SIZE);
}
