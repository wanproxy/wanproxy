/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

	template<class C, typename T>
	static void append(C *out, const T& in)
	{
		T swapped = encode(in);
		out->append(&swapped);
	}

	template<class C, typename T>
	static void extract(T *out, const C *in)
	{
		T wire;
		in->extract(&wire);
		*out = decode(wire);
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

	template<class C, typename T>
	static void append(C *out, const T& in)
	{
		out->append(&in);
	}

	template<class C, typename T>
	static void extract(T *out, const C *in)
	{
		in->extract(out);
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
