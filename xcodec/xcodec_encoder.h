#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcbackref.h>
#include <xcodec/xcdb.h>
#include <xcodec/xcodec_encoder_match.h>

class XCodecEncoder {
	friend class XCodecEncoderMatch;

	class Encoder {
		class Data {
			XCodecEncoder *encoder_;
			Buffer escape_data_;
			bool reference_;
			uint64_t hash_;
			Buffer output_;
		public:
			Data(XCodecEncoder *encoder, uint64_t hash)
			: encoder_(encoder),
			  escape_data_(),
			  reference_(true),
			  hash_(hash),
			  output_()
			{ }

			Data(XCodecEncoder *encoder, const Buffer& escape_data)
			: encoder_(encoder),
			  escape_data_(escape_data),
			  reference_(false),
			  hash_(),
			  output_()
			{
				encode();
			}

			Data(XCodecEncoder *encoder, const Buffer& escape_data, uint64_t hash)
			: encoder_(encoder),
			  escape_data_(escape_data),
			  reference_(true),
			  hash_(hash),
			  output_()
			{ }

			~Data()
			{
				ASSERT(escape_data_.empty());
				ASSERT(output_.empty());
			}

		private:
			struct escape_p {
				bool operator() (uint8_t ch) const {
					return (XCODEC_CHAR_SPECIAL(ch));
				}
			};

			void encode_escape_data(Buffer *output)
			{
				if (escape_data_.empty())
					return;
				escape_data_.escape(XCODEC_ESCAPE_CHAR,
						    escape_p());
				output->append(escape_data_);
				escape_data_.clear();
			}

			void encode_reference(Buffer *output)
			{
				if (!reference_)
					return;

				uint8_t b;

				if (encoder_->backref_.present(hash_, &b)) {
					output->append(XCODEC_BACKREF_CHAR);
					output->append(b);
					return;
				}

				BufferSegment *seg;
				seg = encoder_->database_->lookup(hash_);
				encoder_->backref_.declare(hash_, seg);

				uint64_t hash = LittleEndian::encode(hash_);

				output->append(XCODEC_HASHREF_CHAR);
				output->append((const uint8_t *)&hash,
					       sizeof hash);
				reference_ = false;
			}

			void encode(void)
			{
				ASSERT(output_.empty());
				encode_escape_data(&output_);
				encode_reference(&output_);
			}
		public:
			void encode(Buffer *output)
			{
				if (!output_.empty()) {
					output->append(output_);
					output_.clear();
					return;
				}
				encode_escape_data(output);
				encode_reference(output);
			}
		};

		LogHandle log_;
		XCodecEncoder *encoder_;
		Buffer output_;
		XCDatabase::segment_hash_map_t declarations_;
		std::deque<Data *> data_;
	public:
		Encoder(const LogHandle& log, XCodecEncoder *encoder)
		: log_(log + "/encoder"),
		  encoder_(encoder),
		  output_(),
		  declarations_(),
		  data_()
		{ }

		~Encoder()
		{
			ASSERT(declarations_.empty());
			ASSERT(data_.empty());
			ASSERT(output_.empty());
		}

		void escape(Buffer *buf)
		{
			ASSERT(!buf->empty());
			Data *data = new Data(encoder_, *buf);
			buf->clear();
			data_.push_back(data);
		}

		void declare(uint64_t hash, BufferSegment *seg)
		{
			/*
			 * If we have filled an entire window of declarations,
			 * we need to mixdown all of our pending data right
			 * now, so that we are optimizing our use of BACKREFs.
			 */
			if (declarations_.size() == XCBACKREF_COUNT) {
				encode();
				ASSERT(!output_.empty());
				ASSERT(declarations_.empty());
				ASSERT(data_.empty());
			}
			ASSERT(declarations_.find(hash) ==
			       declarations_.end());
			seg->ref();
			declarations_[hash] = seg;
		}

		void reference(uint64_t hash)
		{
			Data *data = new Data(encoder_, hash);
			data_.push_back(data);
		}

		void reference(Buffer *buf, uint64_t hash)
		{
			Data *data = new Data(encoder_, *buf, hash);
			buf->clear();
			data_.push_back(data);
		}

		void encode(void)
		{
			if (!declarations_.empty()) {
				XCDatabase::segment_hash_map_t::iterator it;

				while ((it = declarations_.begin()) !=
				       declarations_.end()) {
					uint64_t hash = it->first.hash_;
					BufferSegment *seg = it->second;

					encoder_->backref_.declare(hash, seg);

					hash = LittleEndian::encode(hash);

					output_.append(XCODEC_DECLARE_CHAR);
					output_.append((const uint8_t *)&hash,
						       sizeof hash);
					output_.append(seg);
					seg->unref();
					declarations_.erase(it);
				}
			}

			while (!data_.empty()) {
				Data *data = data_.front();
				data_.pop_front();
				data->encode(&output_);
				delete data;
			}
		}

		void takebuf(Buffer *buf)
		{
			buf->append(output_);
			output_.clear();
		}
	};

	LogHandle log_;
	XCodec *codec_;
	XCBackref backref_;
	XCDatabase *database_;
	Encoder encoder_;

public:
	XCodecEncoder(XCodec *codec)
	: log_("/xcodec/encoder"),
	  codec_(codec),
	  backref_(),
	  database_(codec->database_),
	  encoder_(log_, this)
	{ }

	~XCodecEncoder()
	{ }

private:
	bool findmatches(Buffer *buf, std::vector<XCodecEncoderMatch>& matches)
	{
		XCHash<XCODEC_CHUNK_LENGTH> hash;
		bool have_secondary;

		have_secondary = false;

		Buffer::SegmentIterator iter = buf->segments();
		unsigned n = 0;
		while (!iter.end()) {
			const BufferSegment *seg = *iter;
			unsigned j;

			if (n >= XCODEC_CHUNK_LENGTH + XCODEC_WINDOW_SIZE)
				break;

			for (j = 0; j < seg->length(); j++) {
				hash.roll(seg->data()[j]);

				if (++n < XCODEC_CHUNK_LENGTH)
					continue;
				if (n >= XCODEC_CHUNK_LENGTH + XCODEC_WINDOW_SIZE)
					break;

				unsigned o = n - XCODEC_CHUNK_LENGTH;
				XCodecEncoderMatch match(this, buf, o, hash);
				if (match.exists()) {
					if (match.match()) {
						matches.clear();
						matches.push_back(match);
						return (true);
					}
				} else if (!have_secondary) {
					matches.push_back(match);
					have_secondary = true;
				}
			}
			iter.next();
		}
		return (false);
	}

public:
	void encode(Buffer *output, Buffer *input)
	{
		encode(input);
		ASSERT(input->length() == 0);
		encoder_.takebuf(output);
	}

private:
	void encode(Buffer *input)
	{
		std::vector<XCodecEncoderMatch> matches;

		matches.reserve(1);

		while (!input->empty()) {
			if (input->length() < XCODEC_CHUNK_LENGTH) {
				encoder_.escape(input);
				continue;
			}

			/*
			 * If we find a primary match (i.e., one that already
			 * exists), use it.
			 */
			if (findmatches(input, matches)) {
				Buffer skipped;

				XCodecEncoderMatch& match = matches.front();
				uint64_t sum = match.use(&skipped);

				encoder_.reference(&skipped, sum);
				matches.clear();
				continue;
			}
		
			/*
			 * If we found no secondary matches, that means all of
			 * the possible hashes in the window collided.  This
			 * should never happen unless our window is shortened.
			 * Give up and just escape this data.
			 */
			if (matches.empty()) {
				INFO(log_ + "/findmatches") << "Could not find any secondary matches in window.";

				Buffer skipped;

				skipped.append(input, XCODEC_CHUNK_LENGTH);
				input->skip(XCODEC_CHUNK_LENGTH);
				encoder_.escape(&skipped);
				continue;
			}

			/*
			 * Take the first usable hash we found and define it.
			 */
			XCodecEncoderMatch& match = matches.front();
			ASSERT(!match.exists());

			BufferSegment *seg;
			Buffer skipped;
			uint64_t sum = match.define(&skipped, &seg);

			encoder_.declare(sum, seg);
			seg->unref();
			encoder_.reference(&skipped, sum);
			matches.clear();
			continue;
		}
		encoder_.encode();
	}
};

#endif /* !XCODEC_ENCODER_H */
