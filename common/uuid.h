#ifndef	COMMON_UUID_H
#define	COMMON_UUID_H

/*
 * We use strings rather than the binary form to hide the fact that libuuid
 * lacks endian-aware encoding/decoding.
 */

struct UUID {
	std::string string_;

	bool decode(Buffer *);
	bool encode(Buffer *);
	void generate(void);
};

#endif /* !COMMON_UUID_H */
