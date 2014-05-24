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

#include <sys/uio.h>
#include <limits.h>

#include <iostream>

#include <common/limits.h>

#include <common/buffer.h>

#if BUFFER_SEGMENT_CACHE_LIMIT > 0
std::deque<BufferSegment *> BufferSegment::segment_cache;
#endif

size_t
Buffer::fill_iovec(struct iovec *iov, size_t niov) const
{
	if (niov == 0)
		return (0);

	if (niov > IOV_MAX)
		niov = IOV_MAX;

	segment_list_t::const_iterator iter = data_.begin();
	size_t iovcnt = 0;
	unsigned i;

	for (i = 0; i < niov; i++) {
		if (iter == data_.end())
			break;
		const BufferSegment *seg = *iter;
		iov[i].iov_base = (void *)(uintptr_t)seg->data();
		iov[i].iov_len = seg->length();
		iovcnt++;
		iter++;
	}
	return (iovcnt);
}

std::string
Buffer::hexdump(unsigned start) const
{
	static const char hexchars[] = "0123456789abcdef";

	std::string hex;
	std::string vis;

	segment_list_t::const_iterator iter = data_.begin();
	while (iter != data_.end()) {
		const BufferSegment *seg = *iter++;
		const uint8_t *p;

		for (p = seg->data(); p < seg->end(); p++) {
			hex += hexchars[(*p & 0xf0) >> 4];
			hex += hexchars[*p & 0xf];

			if (isprint(*p))
				vis += (char)*p;
			else
				vis += '.';
		}
	}

	std::string dump;
	unsigned cur = 0;

	for (;;) {
		unsigned offset = start + cur;
		std::string str;
		unsigned n;

		while (offset != 0) {
			str = hexchars[offset & 0xf] + str;
			offset >>= 4;
		}

		while (str.length() < 8)
			str = '0' + str;

		dump += str;

		if (cur == length_) {
			break;
		}

		/*
		 * Print hex.
		 */
		for (n = 0; n < 16; n++) {
			if (n % 8 == 0)
				dump += " ";
			dump += " ";
			if (cur + n < length_) {
				dump += hex[n * 2];
				dump += hex[n * 2 + 1];
			} else {
				hex = "";
				dump += "  ";
			}
		}
		if (hex != "")
			hex = hex.substr(32);

		dump += "  |";
		for (n = 0; cur < length_ && n < 16; n++) {
			dump += vis[n];
			cur++;
		}
		if (cur < length_)
			vis = vis.substr(16);
		dump += "|";

		if (cur == length_ && cur % 16 == 0)
			break;

		dump += "\n";
	}

	return (dump);
}

std::ostream&
operator<< (std::ostream& os, const Buffer *buf)
{
	std::string str;
	buf->extract(str);
	return (os << str);
}

std::ostream&
operator<< (std::ostream& os, const Buffer& buf)
{
	return (os << &buf);
}
