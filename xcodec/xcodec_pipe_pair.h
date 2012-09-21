#ifndef	XCODEC_XCODEC_PIPE_PAIR_H
#define	XCODEC_XCODEC_PIPE_PAIR_H

#include <io/pipe/pipe_producer.h>
#include <io/pipe/pipe_producer_wrapper.h>

#include <xcodec/xcodec_decoder.h>

enum XCodecPipePairType {
	XCodecPipePairTypeClient,
	XCodecPipePairTypeServer,
};

class XCodecPipePair : public PipePair {
	LogHandle log_;
	XCodec *codec_;
	XCodecPipePairType type_;

	/*
	 * XXX
	 * Have N encoders and decoders (and, critically, caches), one for each
	 * ''level'', to do multi-level encoding.
	 */
	XCodecDecoder *decoder_;
	XCodecCache *decoder_cache_;
	std::set<uint64_t> decoder_unknown_hashes_;
	bool decoder_received_eos_;
	bool decoder_received_eos_ack_;
	bool decoder_sent_eos_;
	Buffer decoder_buffer_;
	Buffer decoder_frame_buffer_;
	PipeProducerWrapper<XCodecPipePair> *decoder_pipe_;

	XCodecEncoder *encoder_;
	bool encoder_produced_eos_;
	bool encoder_sent_eos_;
	bool encoder_sent_eos_ack_;
	PipeProducerWrapper<XCodecPipePair> *encoder_pipe_;
public:
	XCodecPipePair(const LogHandle& log, XCodec *codec, XCodecPipePairType type)
	: log_(log + "/xcodec"),
	  codec_(codec),
	  type_(type),
	  decoder_(NULL),
	  decoder_cache_(NULL),
	  decoder_unknown_hashes_(),
	  decoder_received_eos_(false),
	  decoder_received_eos_ack_(false),
	  decoder_sent_eos_(false),
	  decoder_buffer_(),
	  decoder_frame_buffer_(),
	  decoder_pipe_(NULL),
	  encoder_(NULL),
	  encoder_produced_eos_(false),
	  encoder_sent_eos_(false),
	  encoder_sent_eos_ack_(false),
	  encoder_pipe_(NULL)
	{
		decoder_pipe_ = new PipeProducerWrapper<XCodecPipePair>(log_ + "/decoder", this, &XCodecPipePair::decoder_consume);
		encoder_pipe_ = new PipeProducerWrapper<XCodecPipePair>(log_ + "/encoder", this, &XCodecPipePair::encoder_consume);
	}

	~XCodecPipePair()
	{
		if (decoder_ != NULL) {
			delete decoder_;
			decoder_ = NULL;
		}

		if (decoder_pipe_ != NULL) {
			delete decoder_pipe_;
			decoder_pipe_ = NULL;
		}

		if (encoder_ != NULL) {
			delete encoder_;
			encoder_ = NULL;
		}

		if (encoder_pipe_ != NULL) {
			delete encoder_pipe_;
			encoder_pipe_ = NULL;
		}
	}

private:
	void decoder_consume(Buffer *);

	void decoder_error(void)
	{
		ERROR(log_) << "Unrecoverable error in decoder.";
		decoder_pipe_->produce_error();
	}

	void decoder_produce(Buffer *buf)
	{
		ASSERT(log_, !buf->empty());
		decoder_pipe_->produce(buf);
	}

	void decoder_produce_eos(Buffer *buf = NULL)
	{
		decoder_pipe_->produce_eos(buf);
	}

	void encoder_consume(Buffer *);

	void encoder_error(void)
	{
		ERROR(log_) << "Unrecoverable error in encoder.";
		encoder_pipe_->produce_error();
	}

	void encoder_produce(Buffer *buf)
	{
		ASSERT(log_, !buf->empty());
		encoder_pipe_->produce(buf);
	}

	void encoder_produce_eos(Buffer *buf = NULL)
	{
		encoder_pipe_->produce_eos(buf);
	}

public:
	Pipe *get_incoming(void)
	{
		switch (type_) {
		case XCodecPipePairTypeClient:
			return (encoder_pipe_);
		case XCodecPipePairTypeServer:
			return (decoder_pipe_);
		default:
			NOTREACHED(log_);
		}
	}

	Pipe *get_outgoing(void)
	{
		switch (type_) {
		case XCodecPipePairTypeClient:
			return (decoder_pipe_);
		case XCodecPipePairTypeServer:
			return (encoder_pipe_);
		default:
			NOTREACHED(log_);
		}
	}
};

#endif /* !XCODEC_XCODEC_PIPE_PAIR_H */
