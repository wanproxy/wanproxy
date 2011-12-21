#ifndef	COMMON_UUID_UUID_H
#define	COMMON_UUID_UUID_H

/*
 * We use strings rather than the binary form to hide the fact that libuuid
 * lacks endian-aware encoding/decoding.
 */
#define	UUID_SIZE	36

struct UUID {
	std::string string_;

	UUID(void)
	: string_("")
	{ }

	bool decode(Buffer *);

	bool encode(Buffer *buf) const
	{
		ASSERT("/uuid", string_.length() == UUID_SIZE);
		buf->append(string_);

		return (true);
	}

	void generate(void);

	bool operator< (const UUID& b) const
	{
		return (string_ < b.string_);
	}
};

#endif /* !COMMON_UUID_UUID_H */
