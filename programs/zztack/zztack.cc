#include <sys/types.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <map>
#include <vector>

#include <common/buffer.h>
#include <common/endian.h>
#include <common/timer.h>

#include <zzcodec/zzcodec.h>

#define	ZZTACK_IOVEC_SIZE	(IOV_MAX)

enum FileAction {
	None, Compress, Decompress
};

static Timer codec_timer;

static void compress(int, int);
static void decompress(int, int);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void process_files(int, char *[], FileAction, bool);
static void time_samples(void);
static void time_stats(void);
static void usage(void);

int
main(int argc, char *argv[])
{
	bool timers, quiet_output, samples;
	FileAction action;
	int ch;

	action = None;
	quiet_output = false;
	samples = false;
	timers = false;

	while ((ch = getopt(argc, argv, "?cdQST")) != -1) {
		switch (ch) {
		case 'c':
			action = Compress;
			break;
		case 'd':
			action = Decompress;
			break;
		case 'Q':
			quiet_output = true;
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
		process_files(argc, argv, action, quiet_output);
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
compress(int ifd, int ofd)
{
	Buffer input, output;

	while (fill(ifd, &input)) {
		codec_timer.start();
		ZZCodec::instance()->encode(&output, &input);
		codec_timer.stop();
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static void
decompress(int ifd, int ofd)
{
	Buffer input, output;

	while (fill(ifd, &input)) {
		codec_timer.start();
		if (!ZZCodec::instance()->decode(&output, &input)) {
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
	BufferSegment *segments[ZZTACK_IOVEC_SIZE];
	struct iovec iov[ZZTACK_IOVEC_SIZE];
	BufferSegment *seg;
	ssize_t len;
	unsigned i;

	for (i = 0; i < ZZTACK_IOVEC_SIZE; i++) {
		seg = new BufferSegment();
		iov[i].iov_base = seg->head();
		iov[i].iov_len = BUFFER_SEGMENT_SIZE;
		segments[i] = seg;
	}

	len = readv(fd, iov, ZZTACK_IOVEC_SIZE);
	if (len == -1)
		HALT("/fill") << "read failed.";
	for (i = 0; i < ZZTACK_IOVEC_SIZE; i++) {
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

	if (fd == -1) {
		output->clear();
		return;
	}

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
process_file(int ifd, int ofd, FileAction action)
{
	switch (action) {
	case Compress:
		compress(ifd, ofd);
		break;
	case Decompress:
		decompress(ifd, ofd);
		break;
	default:
		NOTREACHED();
	}
}

static void
process_files(int argc, char *argv[], FileAction action, bool quiet)
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		if (quiet)
			ofd = -1;
		else
			ofd = STDOUT_FILENO;
		process_file(ifd, ofd, action);
	} else {
		while (argc--) {
			const char *file = *argv++;
			
			ifd = open(file, O_RDONLY);
			ASSERT(ifd != -1);
			if (quiet)
				ofd = -1;
			else
				ofd = STDOUT_FILENO;

			process_file(ifd, ofd, action);
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
"usage: zztack [-QST] -c [file ...]\n"
"       zztack [-QST] -d [file ...]\n");
	exit(1);
}
