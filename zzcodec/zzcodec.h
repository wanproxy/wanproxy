#ifndef	ZZCODEC_H
#define	ZZCODEC_H

#define	ZZCODEC_COLUMNS	(BUFFER_SEGMENT_SIZE)
#define	ZZCODEC_ROWS	(BUFFER_SEGMENT_SIZE)

class ZZCodec {
	LogHandle log_;
	unsigned transform_[ZZCODEC_COLUMNS * ZZCODEC_ROWS];

	ZZCodec(void);

	~ZZCodec()
	{ }

public:
	bool decode(Buffer *, Buffer *) const;
	void encode(Buffer *, Buffer *) const;

	static ZZCodec *instance(void)
	{
		static ZZCodec *instance_;

		if (instance_ == NULL)
			instance_ = new ZZCodec();
		return (instance_);
	}
};

#endif /* !ZZCODEC_H */
