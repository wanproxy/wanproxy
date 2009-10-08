#ifndef	XCODEC_ENCODER_STATS_H
#define	XCODEC_ENCODER_STATS_H

#if !defined(XCODEC_STATS)
#error "XCODEC_STATS must be defined to use stats."
#endif

class Action;

class XCodecEncoderStats {
	LogHandle log_;

	Action *stop_action_;
	Action *timeout_action_;

	uint64_t input_bytes_;
	uint64_t output_bytes_;

	std::pair<uint64_t, uint64_t> best_ratio_;
	std::pair<uint64_t, uint64_t> largest_reduction_;

	XCodecEncoderStats(void);
	~XCodecEncoderStats();

	void stop_complete(void);
	void timeout_complete(void);

	void display(void);
public:
	static void record(uint64_t input_bytes, uint64_t output_bytes)
	{
		static XCodecEncoderStats stats;

		if (input_bytes == 0)
			return;
		ASSERT(output_bytes != 0);

		stats.input_bytes_ += input_bytes;
		stats.output_bytes_ += output_bytes;

		if (input_bytes < output_bytes)
			return;

		if ((stats.largest_reduction_.first -
		     stats.largest_reduction_.second) <
		    (input_bytes - output_bytes)) {
			stats.largest_reduction_.first = input_bytes;
			stats.largest_reduction_.second = output_bytes;
		}

		if (stats.best_ratio_.first == 0) {
			stats.best_ratio_.first = input_bytes;
			stats.best_ratio_.second = output_bytes;
		} else {
			double current_ratio =
				(double)stats.best_ratio_.second /
				(double)stats.best_ratio_.first;
			double this_ratio = (double)output_bytes /
				(double)input_bytes;
			if (this_ratio < current_ratio ||
			    (this_ratio == current_ratio &&
			     input_bytes > stats.best_ratio_.first)) {
				stats.best_ratio_.first = input_bytes;
				stats.best_ratio_.second = output_bytes;
			}
		}
	}
};

#endif /* !XCODEC_ENCODER_STATS_H */
