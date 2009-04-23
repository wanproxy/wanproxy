#ifndef	ZZCODEC_H
#define	ZZCODEC_H

#define	ZZCODEC_COLUMNS	(128)
#define	ZZCODEC_ROWS	(128)

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
