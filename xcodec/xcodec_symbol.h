/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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

#ifndef	XCODEC_XCODEC_SYMBOL_H
#define	XCODEC_XCODEC_SYMBOL_H

union XCodecSymbol {
	struct {
		uint64_t reserved_:3;
		uint64_t hash_:61;
	} fields_;
	uint64_t word_;

	XCodecSymbol(void)
	: word_(0)
	{ }

	void append(Buffer *buf)
	{
		uint64_t word = BigEndian::encode(word_);

		buf->append(&word);
	}

	void extract(Buffer *buf, unsigned offset = 0)
	{
		ASSERT(log_, buf->length() >= sizeof word_ + offset);

		uint64_t word;

		buf->extract(&word, offset);
		word_ = BigEndian::decode(word);
	}

	void moveout(Buffer *buf, unsigned offset = 0)
	{
		ASSERT(log_, buf->length() >= sizeof word_ + offset);
		if (offset != 0)
			buf->skip(offset);
		buf->extract(buf);
	}
};

#endif /* !XCODEC_XCODEC_SYMBOL_H */
