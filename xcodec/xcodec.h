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

#ifndef	XCODEC_XCODEC_H
#define	XCODEC_XCODEC_H

#define	XCODEC_MAGIC		((uint8_t)0xf1)	/* Magic!  */

/*
 * Usage:
 * 	<MAGIC> <OP_ESCAPE>
 *
 * Effects:
 * 	A literal XCODEC_MAGIC appears in the output stream.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_OP_ESCAPE	((uint8_t)0x00)

/*
 * Usage:
 * 	<MAGIC> <OP_EXTRACT> data[uint8_t x XCODEC_SEGMENT_LENGTH]
 *
 * Effects:
 * 	The `data' is hashed, the hash is associated with the data if possible
 * 	and the data is inserted into the output stream.
 *
 * 	If other data is already known by the hash of `data', error will be
 * 	indicated from the decoder.
 *
 * Side-effects:
 * 	The data is put into the backref FIFO.
 */
#define	XCODEC_OP_EXTRACT	((uint8_t)0x01)

/*
 * Usage:
 * 	<MAGIC> <OP_REF> hash[uint64_t]
 *
 * Effects:
 * 	The data associated with the hash `hash' is looked up and inserted into
 * 	the output stream if possible.
 *
 * 	If the `hash' is not known, an OP_ASK will be sent in response.
 *
 * Side-effects:
 * 	The data is put into the backref FIFO.
 */
#define	XCODEC_OP_REF		((uint8_t)0x02)

/*
 * Usage:
 * 	<MAGIC> <OP_BACKREF> index[uint8_t]
 *
 * Effects:
 * 	The data at index `index' in the backref FIFO is inserted into the
 * 	output stream.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_OP_BACKREF	((uint8_t)0x03)

#define	XCODEC_SEGMENT_LENGTH	(2048)

class XCodecCache;

class XCodec {
	LogHandle log_;
	XCodecCache *cache_;
public:
	XCodec(XCodecCache *database)
	: log_("/xcodec"),
	  cache_(database)
	{ }

	~XCodec()
	{ }

	XCodecCache *cache(void)
	{
		return (cache_);
	}
};

#endif /* !XCODEC_XCODEC_H */
