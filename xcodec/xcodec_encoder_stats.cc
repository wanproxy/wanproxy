#include <common/buffer.h> /* XXX For class Event */

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>
#include <event/timeout.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder_stats.h>

#define	DISPLAY_MS	5000

XCodecEncoderStats::XCodecEncoderStats(void)
: log_("/xcodec/encoder/stats"),
  stop_action_(NULL),
  timeout_action_(NULL),
  input_bytes_(0),
  output_bytes_(0),
  best_ratio_(0, 0),
  largest_reduction_(0, 0)
{
	Callback *scb = callback(this, &XCodecEncoderStats::stop_complete);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);

	Callback *tcb = callback(this, &XCodecEncoderStats::timeout_complete);
	timeout_action_ = EventSystem::instance()->timeout(DISPLAY_MS, tcb);
}

XCodecEncoderStats::~XCodecEncoderStats()
{
	ASSERT(stop_action_ == NULL);
	ASSERT(timeout_action_ == NULL);
}

void
XCodecEncoderStats::stop_complete(void)
{
	stop_action_->cancel();
	stop_action_ = NULL;

	if (timeout_action_ != NULL) {
		timeout_action_->cancel();
		timeout_action_ = NULL;
	}

	display();
}

void
XCodecEncoderStats::timeout_complete(void)
{
	timeout_action_->cancel();
	timeout_action_ = NULL;

	display();

	Callback *cb = callback(this, &XCodecEncoderStats::timeout_complete);
	timeout_action_ = EventSystem::instance()->timeout(DISPLAY_MS, cb);
}

void
XCodecEncoderStats::display(void)
{
	INFO(log_) << "Total input bytes: " << input_bytes_ << "; Total output bytes: " << output_bytes_ << "; Overall ratio: " << ((double)input_bytes_ / (double)output_bytes_) << ":1";
	if (best_ratio_.first != 0) {
		INFO(log_) << "Best ratio: " << ((double)best_ratio_.first / (double)best_ratio_.second) << ":1 (" << best_ratio_.first << ":" << best_ratio_.second << ")";
	}
	if (largest_reduction_.first != 0) {
		INFO(log_) << "Largest reduction: " << ((double)largest_reduction_.first / (double)largest_reduction_.second) << ":1 (" << largest_reduction_.first << ":" << largest_reduction_.second << ")";
	}
}
