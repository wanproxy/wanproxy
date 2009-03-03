#ifndef	XCODEC_ENCODER_MATCH_H
#define	XCODEC_ENCODER_MATCH_H

class XCodecEncoder;

class XCodecEncoderMatch {
	XCodecEncoder *encoder_;
	Buffer *src_;
	unsigned offset_;
	uint64_t hash_;
	BufferSegment *old_;
public:
	XCodecEncoderMatch(XCodecEncoder *, Buffer *, unsigned,
			   const XCHash<XCODEC_CHUNK_LENGTH>&);

	XCodecEncoderMatch(const XCodecEncoderMatch& hash)
	: encoder_(hash.encoder_),
	  src_(hash.src_),
	  offset_(hash.offset_),
	  hash_(hash.hash_),
	  old_(NULL)
	{
		old_ = hash.old_;
		if (old_ != NULL)
			old_->ref();
	}
	
	~XCodecEncoderMatch()
	{
		if (old_ != NULL) {
			old_->unref();
			old_ = NULL;
		}
	}

	bool exists(void) const
	{
		return (old_ != NULL);
	}

	bool match(void) const
	{
		ASSERT(exists());

		uint8_t tmp[XCODEC_CHUNK_LENGTH];
		src_->copyout(tmp, offset_, sizeof tmp);

		return (old_->match(tmp, XCODEC_CHUNK_LENGTH));
	}

	uint64_t use(Buffer *skipped)
	{
		ASSERT(match());

		skipped->append(src_, offset_);
		src_->skip(offset_);

		src_->skip(XCODEC_CHUNK_LENGTH);
		return (hash_);
	}

	uint64_t define(Buffer *, BufferSegment **);
};

#endif /* !XCODEC_ENCODER_MATCH_H */
