#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcdb.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>

XCodecDecoder::XCodecDecoder(XCodec *codec)
: log_("/xcodec/decoder"),
  database_(codec->database_),
  backref_()
{ }

XCodecDecoder::~XCodecDecoder()
{ }

/*
 * Decode an XCodec-encoded stream.  Returns false if there was an
 * inconsistency in the stream or something malformed.  Returns true
 * if we were able to process the stream or expect to be able to
 * once more data arrives.  The input buffer is cleared of anything
 * we can process right now.
 */
bool
XCodecDecoder::decode(Buffer *output, Buffer *input)
{
	BufferSegment *seg, *oseg;
	uint64_t hash;
	size_t inlen;
	uint8_t ch;

	inlen = input->length();
	while (inlen != 0) {
		ch = input->peek();

		while (!XCODEC_CHAR_SPECIAL(ch)) {
			inlen--;
			input->skip(1);
			output->append(ch);

			if (inlen == 0)
				break;

			ch = input->peek();
		}

		ASSERT(inlen == input->length());

		if (inlen == 0)
			break;

		switch (ch) {
			/*
			 * A reference to a hash we are expected to have
			 * in our database.
			 */
		case XCODEC_HASHREF_CHAR:
			if (inlen < 9)
				return (true);

			input->moveout((uint8_t *)&hash, 1, sizeof hash);
			hash = LittleEndian::decode(hash);

			seg = database_->lookup(hash);
			if (seg == NULL) {
				ERROR(log_) << "Stream referenced unseen hash: " << hash;
				return (false);
			}
			backref_.declare(hash, seg);

			output->append(seg);
			inlen -= 9;
			break;

			/*
			 * A literal character will follow that would
			 * otherwise have seemed magic.
			 */
		case XCODEC_ESCAPE_CHAR:
			if (inlen < 2)
				return (true);
			input->skip(1);
			output->append(input->peek());
			input->skip(1);
			inlen -= 2;
			break;

			/*
			 * A learning opportunity -- a hash we should put
			 * into our database for future use and the data
			 * it will be used in reference to next time.
			 */
		case XCODEC_DECLARE_CHAR:
			if (inlen < 9 + XCODEC_CHUNK_LENGTH)
				return (true);

			input->moveout((uint8_t *)&hash, 1, sizeof hash);
			hash = LittleEndian::decode(hash);

			input->copyout(&seg, XCODEC_CHUNK_LENGTH);
			input->skip(XCODEC_CHUNK_LENGTH);

			if (XCHash<XCODEC_CHUNK_LENGTH>::hash(seg->data()) != hash) {
				/* XXX Show expected and actual hash?  */
				seg->unref();
				ERROR(log_) << "Data in stream does not have expected hash.";
				return (false);
			}

			/*
			 * Chech whether we already know a hash by this name.
			 */
			oseg = database_->lookup(hash);
			if (oseg != NULL) {
				/*
				 * We already know a hash by this name.  Check
				 * whether it is for the same data.  That would
				 * be nice.  In that case all we need to do is
				 * update the backreference window.
				 */
				if (seg->match(oseg)) {
					seg->unref();
					/*
					 * NB: Use the existing segment since
					 * it's nice to share.
					 */
					backref_.declare(hash, oseg);
					oseg->unref();
				} else {
					seg->unref();
					oseg->unref();
					ERROR(log_) << "Stream includes hash with different local data.";
					return (false);
				}
			} else {
				/*
				 * This is a brand new hash.  Enter it into the
				 * database and the backref window.
				 */
				database_->enter(hash, seg);
				backref_.declare(hash, seg);
				seg->unref();
			}

			inlen -= 9 + XCODEC_CHUNK_LENGTH;
			break;

			/*
			 * A reference to a recently-used hash.  We expect
			 * to see a lot of these whenever we see declarations
			 * since the declarations are desorted in the stream
			 * and are not literally inserted.
			 */
		case XCODEC_BACKREF_CHAR:
			if (inlen < 2)
				return (true);
			input->skip(1);
			ch = input->peek();
			input->skip(1);

			seg = backref_.dereference(ch);
			if (seg == NULL) {
				ERROR(log_) << "Stream included invalid backref.";
				return (false);
			}

			output->append(seg);
			inlen -= 2;
			break;

		default:
			NOTREACHED();
		}
	}

	return (true);
}
