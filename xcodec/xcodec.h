#ifndef	XCODEC_H
#define	XCODEC_H

#define	XCODEC_CHAR_BASE	((uint8_t)0xf0)
#define	XCODEC_CHAR(option)	(XCODEC_CHAR_BASE | (option))
#define	XCODEC_HASHREF_CHAR	(XCODEC_CHAR(0x00))
#define	XCODEC_ESCAPE_CHAR	(XCODEC_CHAR(0x01))
#define	XCODEC_DECLARE_CHAR	(XCODEC_CHAR(0x02))
#define	XCODEC_BACKREF_CHAR	(XCODEC_CHAR(0x03))
#define	XCODEC_CHAR_MASK	(0x03)
#define	XCODEC_CHAR_SPECIAL(ch)	(((ch) & ~XCODEC_CHAR_MASK) == XCODEC_CHAR_BASE)

#define	XCODEC_SEGMENT_LENGTH	(BUFFER_SEGMENT_SIZE)

class XCodecDecoder;
class XCodecEncoder;
class XCodecCache;

class XCodec {
	friend class XCodecDecoder;
	friend class XCodecEncoder;

	LogHandle log_;
	XCodecCache *cache_;
public:
	XCodec(XCodecCache *database)
	: log_("/xcodec"),
	  cache_(database)
	{ }

	~XCodec()
	{ }

	XCodecDecoder *decoder(void);
	XCodecEncoder *encoder(void);
};

#endif /* !XCODEC_H */
