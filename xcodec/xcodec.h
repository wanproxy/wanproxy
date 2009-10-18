#ifndef	XCODEC_H
#define	XCODEC_H

#define	XCODEC_MAGIC	((uint8_t)0xf1)	/* Magic!  */

/*
 * Usage:
 * 	<MAGIC> <OP_HELLO> length[uint8_t] data[uint8_t x length]
 *
 * Effects:
 * 	Must appear at the start of and only at the start of an encoded	stream.
 *
 * Sife-effects:
 * 	Possibly many.
 */
#define	XCODEC_OP_HELLO	((uint8_t)0x00)

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
#define	XCODEC_OP_ESCAPE	((uint8_t)0x01)

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
#define	XCODEC_OP_EXTRACT	((uint8_t)0x02)

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
#define	XCODEC_OP_REF	((uint8_t)0x03)

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
#define	XCODEC_OP_BACKREF	((uint8_t)0x04)

/*
 * Usage:
 * 	<MAGIC> <OP_LEARN> data[uint8_t x XCODEC_SEGMENT_LENGTH]
 *
 * Effects:
 * 	The `data' is hashed, the hash is associated with the data if possible.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_OP_LEARN	((uint8_t)0x05)

/*
 * Usage:
 * 	<MAGIC> <OP_ASK> hash[uint64_t]
 *
 * Effects:
 * 	An OP_LEARN will be sent in response with the data corresponding to the
 * 	hash.
 *
 * 	If the hash is unknown, error will be indicated.
 *
 * Side-effects:
 * 	None.
 */
#define	XCODEC_OP_ASK	((uint8_t)0x06)

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

	/*
	 * These operations can always be completed immediately and will not
	 * change the semantics of any operations before them in a stream.
	 */
	static bool side_effects(uint8_t op)
	{
		switch (op) {
		case XCODEC_OP_ESCAPE:
		case XCODEC_OP_LEARN:
		case XCODEC_OP_ASK:
			return (false);
		default:
			return (true);
		}
	}
};

#endif /* !XCODEC_H */
