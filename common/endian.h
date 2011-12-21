#ifndef	COMMON_ENDIAN_H
#define	COMMON_ENDIAN_H

#if !defined(BYTE_ORDER) && !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#define	LITTLE_ENDIAN	1234
#define	BIG_ENDIAN	4321
#if defined(_LITTLE_ENDIAN)
#define	BYTE_ORDER	LITTLE_ENDIAN
#elif defined(_BIG_ENDIAN)
#define	BYTE_ORDER	BIG_ENDIAN
#else
#error "Can't determine host byte order."
#endif
#endif

#ifndef	BYTE_ORDER
#error "BYTE_ORDER must be defined."
#else
#if BYTE_ORDER == LITTLE_ENDIAN
#elif BYTE_ORDER == BIG_ENDIAN
#else
#error "Unexpected BYTE_ORDER value."
#endif
#endif

struct Endian {
	static uint16_t swap(const uint16_t& in)
	{
		return (((in & 0xff00u) >> 0x08) | ((in & 0x00ffu) << 0x08));
	}

	static uint32_t swap(const uint32_t& in)
	{
		return (((in & 0xff000000u) >> 0x18) |
			((in & 0x00ff0000u) >> 0x08) |
			((in & 0x0000ff00u) << 0x08) |
			((in & 0x000000ffu) << 0x18));
	}

	static uint64_t swap(const uint64_t& in)
	{
		return (((in & 0xff00000000000000ull) >> 0x38) |
			((in & 0x00ff000000000000ull) >> 0x28) |
			((in & 0x0000ff0000000000ull) >> 0x18) |
			((in & 0x000000ff00000000ull) >> 0x08) |
			((in & 0x00000000ff000000ull) << 0x08) |
			((in & 0x0000000000ff0000ull) << 0x18) |
			((in & 0x000000000000ff00ull) << 0x28) |
			((in & 0x00000000000000ffull) << 0x38));
	}
};

struct SwapEndian {
	template<typename T>
	static T encode(const T& in)
	{
		return (Endian::swap(in));
	}

	template<typename T>
	static T decode(const T& in)
	{
		return (Endian::swap(in));
	}
};

struct HostEndian {
	template<typename T>
	static T encode(const T& in)
	{
		return (in);
	}

	template<typename T>
	static T decode(const T& in)
	{
		return (in);
	}
};

#if BYTE_ORDER == LITTLE_ENDIAN
typedef	HostEndian	LittleEndian;
typedef	SwapEndian	BigEndian;
#elif BYTE_ORDER == BIG_ENDIAN
typedef	SwapEndian	LittleEndian;
typedef	HostEndian	BigEndian;
#endif

#endif /* !COMMON_ENDIAN_H */
