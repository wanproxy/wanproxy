#include <uuid/uuid.h>

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
	int rv = uuid_parse(string_.c_str(), uuid);
	if (rv == -1)
		return (false);
	ASSERT(rv == 0);

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

	char str[UUID_SIZE + 1];

	uuid_generate(uuid);
	uuid_unparse(uuid, str);
	string_ = str;
	ASSERT(string_.length() == UUID_SIZE);
}
