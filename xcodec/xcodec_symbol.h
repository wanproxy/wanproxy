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
