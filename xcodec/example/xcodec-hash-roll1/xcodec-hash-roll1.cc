#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/stream_handle.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_hash.h>

class Sink {
	LogHandle log_;

	StreamHandle fd_;
	XCodecHash hash_;
	unsigned length_;
	Action *action_;
public:
	Sink(int fd)
	: log_("/sink"),
	  fd_(fd),
	  hash_(),
	  length_(0),
	  action_(NULL)
	{
		EventCallback *cb = callback(this, &Sink::read_complete);
		action_ = fd_.read(0, cb);
	}

	~Sink()
	{
		ASSERT(log_, action_ == NULL);
	}

	void read_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		while (!e.buffer_.empty()) {
			BufferSegment *seg;
			e.buffer_.moveout(&seg);

			const uint8_t *p, *q = seg->end();
			if (length_ == XCODEC_SEGMENT_LENGTH) {
				for (p = seg->data(); p < q; p++) {
					hash_.roll(*p);
				}
			} else {
				for (p = seg->data(); p < q; p++) {
					if (length_ == XCODEC_SEGMENT_LENGTH) {
						hash_.roll(*p);
					} else {
						hash_.add(*p);
						length_++;
					}
				}
			}
			seg->unref();
		}

		if (e.type_ == Event::EOS) {
			SimpleCallback *cb = callback(this, &Sink::close_complete);
			action_ = fd_.close(cb);
			return;
		}

		EventCallback *cb = callback(this, &Sink::read_complete);
		action_ = fd_.read(0, cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;
	}
};

int
main(void)
{
	Sink sink(STDIN_FILENO);

	event_main();
}
