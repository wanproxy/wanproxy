#include <sys/types.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <deque>
#include <map>
#include <vector>

#include <common/buffer.h>
#include <common/endian.h>
#include <event/timer.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>

#define	TACK_IOVEC_SIZE	(IOV_MAX)

enum FileAction {
	None, Compress, Decompress
};

static Timer codec_timer;

static void compress(int, int, XCodec *);
static void decompress(int, int, XCodec *);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void process_files(int, char *[], FileAction, XCodec *);
static void time_samples(void);
static void time_stats(void);
static void usage(void);

int
main(int argc, char *argv[])
{
	XCDatabase database("/database");
	XCodec codec("/main", &database);
	bool timers, samples;
	FileAction action;
	int ch;

	action = None;
	samples = false;
	timers = false;

	while ((ch = getopt(argc, argv, "?cdST")) != -1) {
		switch (ch) {
		case 'c':
			action = Compress;
			break;
		case 'd':
			action = Decompress;
			break;
		case 'S':
			samples = true;
			break;
		case 'T':
			timers = true;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	switch (action) {
	case None:
		usage();
	case Compress:
	case Decompress:
		process_files(argc, argv, action, &codec);
		if (samples)
			time_samples();
		if (timers)
			time_stats();
		break;
	default:
		NOTREACHED();
	}

	return (0);
}

static void
compress(int ifd, int ofd, XCodec *codec)
{
	XCodecEncoder encoder(codec);
	Buffer input, output;

	while (fill(ifd, &input)) {
		codec_timer.start();
		encoder.encode(&output, &input);
		codec_timer.stop();
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static void
decompress(int ifd, int ofd, XCodec *codec)
{
	XCodecDecoder decoder(codec);
	Buffer input, output;

	while (fill(ifd, &input)) {
		codec_timer.start();
		if (!decoder.decode(&output, &input)) {
			ERROR("/decompress") << "Decode failed.";
			return;
		}
		codec_timer.stop();
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static bool
fill(int fd, Buffer *input)
{
	BufferSegment *segments[TACK_IOVEC_SIZE];
	struct iovec iov[TACK_IOVEC_SIZE];
	BufferSegment *seg;
	ssize_t len;
	unsigned i;

	for (i = 0; i < TACK_IOVEC_SIZE; i++) {
		seg = new BufferSegment();
		iov[i].iov_base = seg->head();
		iov[i].iov_len = BUFFER_SEGMENT_SIZE;
		segments[i] = seg;
	}

	len = readv(fd, iov, TACK_IOVEC_SIZE);
	if (len == -1)
		HALT("/fill") << "read failed.";
	for (i = 0; i < TACK_IOVEC_SIZE; i++) {
		seg = segments[i];
		if (len > 0) {
			if (len > BUFFER_SEGMENT_SIZE) {
				seg->set_length(BUFFER_SEGMENT_SIZE);
				len -= BUFFER_SEGMENT_SIZE;
			} else {
				seg->set_length((size_t)len);
				len = 0;
			}
			input->append(seg);
		}
		seg->unref();
	}
	if (input->empty())
		return (false);
	return (true);
}

static void
flush(int fd, Buffer *output)
{
	ssize_t len;

	for (;;) {
		Buffer::SegmentIterator iter = output->segments();
		if (iter.end())
			break;

		const BufferSegment *seg = *iter;

		len = write(fd, seg->data(), seg->length());
		if (len != (ssize_t)seg->length())
			HALT("/output") << "write failed.";
		output->skip((size_t)len);
	}
	ASSERT(output->empty());
}

static void
process_file(int ifd, int ofd, FileAction action, XCodec *codec)
{
	switch (action) {
	case Compress:
		compress(ifd, ofd, codec);
		break;
	case Decompress:
		decompress(ifd, ofd, codec);
		break;
	default:
		NOTREACHED();
	}
}

static void
process_files(int argc, char *argv[], FileAction action, XCodec *codec)
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		ofd = STDOUT_FILENO;
		process_file(ifd, ofd, action, codec);
	} else {
		while (argc--) {
			const char *file = *argv++;
			
			ifd = open(file, O_RDONLY);
			ASSERT(ifd != -1);
			ofd = STDOUT_FILENO;

			process_file(ifd, ofd, action, codec);
		}
	}
}

static void
time_samples(void)
{
	std::vector<uintmax_t> samples = codec_timer.samples();
	std::vector<uintmax_t>::iterator it;

	for (it = samples.begin(); it != samples.end(); ++it)
		fprintf(stderr, "%ju\n", *it);
}

static void
time_stats(void)
{
	std::vector<uintmax_t> samples = codec_timer.samples();
	std::vector<uintmax_t>::iterator it;
	LogHandle log("/codec_timer");
	uintmax_t microseconds;

	INFO(log) << samples.size() << " timer samples.";
	microseconds = 0;
	for (it = samples.begin(); it != samples.end(); ++it)
		microseconds += *it;
	INFO(log) << microseconds << " total runtime.";
	INFO(log) << (microseconds / samples.size()) << " mean microseconds/call.";
}

static void
usage(void)
{
	fprintf(stderr,
"usage: tack [-ST] -c [file ...]\n"
"       tack [-ST] -d [file ...]\n");
	exit(1);
}
