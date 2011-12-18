#include <uuid.h>

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
	uint32_t status;
	uuid_from_string(string_.c_str(), &uuid, &status);
	if (status != uuid_s_ok)
		return (false);

	return (true);
}

void
UUID::generate(void)
{
	uuid_t uuid;

	char *p;

	uuid_create(&uuid, NULL);
	uuid_to_string(&uuid, &p, NULL);
	ASSERT("/uuid/libc", p != NULL);
	string_ = p;
	free(p);
	ASSERT("/uuid/libc", string_.length() == UUID_SIZE);
}
