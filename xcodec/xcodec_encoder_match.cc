#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_encoder_match.h>

XCodecEncoderMatch::XCodecEncoderMatch(XCodecEncoder *encoder, Buffer *src,
				       unsigned offset,
				       const XCHash<XCODEC_CHUNK_LENGTH>& hash)
: encoder_(encoder),
  src_(src),
  offset_(offset),
  hash_(hash.mix())
{
	old_ = encoder_->database_->lookup(hash_);
}

uint64_t
XCodecEncoderMatch::define(Buffer *skipped, BufferSegment **segp)
{
	ASSERT(!exists());

	skipped->append(src_, offset_);
	src_->skip(offset_);

	BufferSegment *seg;
	src_->copyout(&seg, XCODEC_CHUNK_LENGTH);
	src_->skip(XCODEC_CHUNK_LENGTH);

	encoder_->database_->enter(hash_, seg);
	*segp = seg;

	return (hash_);
}
