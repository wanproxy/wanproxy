/*
 * Copyright (c) 2016 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_DATA_BASE16_H
#define	COMMON_DATA_BASE16_H

/*
 * Base16 (i.e. hex) encoding and decoding for Buffers.
 */
struct Base16 {
	static bool decode(Buffer *out, const Buffer *in)
	{
		if (in->length() % 2 != 0) {
			ERROR("/base16") << "Refusing to decode data with odd number of hex digits.";
			return (false);
		}

		/*
		 * We use a temporary buffer rather than maintaining a
		 * lot of internal state to avoid copies.  The Buffer
		 * code itself much more sophisticated than we need to
		 * be here and gets the job done easily.
		 */
		Buffer encoded(*in);
		Buffer decoded;
		while (encoded.length() >= 2) {
			uint8_t d[2];

			encoded.moveout(d, sizeof d);

			uint8_t b = 0x00;

			if (d[0] >= '0' && d[0] <= '9')
				b |= d[0] - '0';
			else if (d[0] >= 'a' && d[0] <= 'f')
				b |= 0xa + (d[0] - 'a');
			else if (d[0] >= 'A' && d[0] <= 'F')
				b |= 0xa + (d[0] - 'A');
			else {
				ERROR("/base16") << "Could not decode data with malformed hex digit (0 in pair.)";
				return (false);
			}
			b <<= 4;

			if (d[1] >= '0' && d[1] <= '9')
				b |= d[1] - '0';
			else if (d[1] >= 'a' && d[1] <= 'f')
				b |= 0xa + (d[1] - 'a');
			else if (d[1] >= 'A' && d[1] <= 'F')
				b |= 0xa + (d[1] - 'A');
			else {
				ERROR("/base16") << "Could not decode data with malformed hex digit (1 in pair.)";
				return (false);
			}

			decoded.append(b);
		}

		decoded.moveout(out);
		return (true);
	}

	static Buffer encode(const Buffer *in)
	{
		static const char hexchars[] = "0123456789abcdef";
		Buffer out;

		Buffer::SegmentIterator iter(in->segments());
		while (!iter.end()) {
			const BufferSegment *seg = *iter;
			const uint8_t *p;

			for (p = seg->data(); p < seg->end(); p++) {
				out.append((uint8_t)hexchars[(*p & 0xf0) >> 4]);
				out.append((uint8_t)hexchars[*p & 0xf]);
			}

			iter.next();
		}

		return (out);
	}
};

#endif /* !COMMON_DATA_BASE16_H */
