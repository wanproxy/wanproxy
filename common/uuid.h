#ifndef	COMMON_UUID_H
#define	COMMON_UUID_H

/*
 * We use strings rather than the binary form to hide the fact that libuuid
 * lacks endian-aware encoding/decoding.
 */
#define	UUID_SIZE	36

struct UUID {
	std::string string_;

	bool decode(Buffer *);
	bool encode(Buffer *) const;
	void generate(void);

	bool operator< (const UUID& b) const
	{
		return (string_ < b.string_);
	}
};

#endif /* !COMMON_UUID_H */
