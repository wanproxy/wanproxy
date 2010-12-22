#include <sys/types.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include <map>
#include <vector>

#include <common/buffer.h>
#include <common/endian.h>
#include <common/limits.h>
#include <common/timer/timer.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>

enum FileAction {
	None, Compress, Decompress
};

static void compress(int, int, XCodec *, bool, Timer *);
static void decompress(int, int, XCodec *, bool, Timer *);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void process_file(int, int, FileAction, XCodec *, bool, Timer *);
static void process_files(int, char *[], FileAction, XCodec *, bool, bool, Timer *);
static void time_samples(Timer *);
static void time_stats(Timer *);
static void usage(void);

int
main(int argc, char *argv[])
{
	UUID uuid;
	uuid.generate();

	XCodecCache *cache = XCodecCache::lookup(uuid);
	XCodec codec(cache);

	bool codec_timing, quiet_output, samples, verbose;
	FileAction action;
	int ch;

	action = None;
	codec_timing = false;
	quiet_output = false;
	samples = false;
	verbose = false;

	while ((ch = getopt(argc, argv, "?cdvQST")) != -1) {
		switch (ch) {
		case 'c':
			action = Compress;
			break;
		case 'd':
			action = Decompress;
			break;
		case 'v':
			verbose = true;
			break;
		case 'Q':
			quiet_output = true;
			break;
		case 'S':
			samples = true;
			break;
		case 'T':
			codec_timing = true;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (verbose) {
		Log::mask(".?", Log::Debug);
	} else {
		Log::mask(".?", Log::Info);
	}

	Timer codec_timer;

	switch (action) {
	case None:
		usage();
	case Compress:
	case Decompress:
		process_files(argc, argv, action, &codec, quiet_output, codec_timing, &codec_timer);
		if (samples)
			time_samples(&codec_timer);
		if (codec_timing)
			time_stats(&codec_timer);
		break;
	default:
		NOTREACHED();
	}

	return (0);
}

static void
compress(int ifd, int ofd, XCodec *codec, bool timing, Timer *timer)
{
	XCodecEncoder encoder(codec->cache());
	Buffer input, output;

	while (fill(ifd, &input)) {
		if (timing)
			timer->start();
		encoder.encode(&output, &input);
		if (timing)
			timer->stop();
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static void
decompress(int ifd, int ofd, XCodec *codec, bool timing, Timer *timer)
{
	std::set<uint64_t> unknown_hashes;
	XCodecDecoder decoder(codec->cache());
	Buffer input, output;

	(void)codec;

	while (fill(ifd, &input)) {
		if (timing)
			timer->start();
		if (!decoder.decode(&output, &input, unknown_hashes)) {
			ERROR("/decompress") << "Decode failed.";
			return;
		}
		if (timing)
			timer->stop();
		if (!unknown_hashes.empty()) {
			ERROR("/decompress") << "Cannot decode stream with unknown hashes.";
			return;
		}
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static bool
fill(int fd, Buffer *input)
{
	uint8_t data[65536];
	ssize_t len;

	len = read(fd, data, sizeof data);
	if (len == -1)
		HALT("/fill") << "read failed.";
	if (len == 0)
		return (false);
	input->append(data, len);
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
process_file(int ifd, int ofd, FileAction action, XCodec *codec, bool timing, Timer *timer)
{
	switch (action) {
	case Compress:
		compress(ifd, ofd, codec, timing, timer);
		break;
	case Decompress:
		decompress(ifd, ofd, codec, timing, timer);
		break;
	default:
		NOTREACHED();
	}
}

static void
process_files(int argc, char *argv[], FileAction action, XCodec *codec, bool quiet, bool timing, Timer *timer)
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		if (quiet)
			ofd = -1;
		else
			ofd = STDOUT_FILENO;
		process_file(ifd, ofd, action, codec, timing, timer);
	} else {
		while (argc--) {
			const char *file = *argv++;

			ifd = open(file, O_RDONLY);
			ASSERT(ifd != -1);
			if (quiet)
				ofd = -1;
			else
				ofd = STDOUT_FILENO;

			process_file(ifd, ofd, action, codec, timing, timer);
		}
	}
}

static void
time_samples(Timer *timer)
{
	std::vector<uintmax_t> samples = timer->samples();
	std::vector<uintmax_t>::iterator it;

	for (it = samples.begin(); it != samples.end(); ++it)
		fprintf(stderr, "%ju\n", *it);
}

static void
time_stats(Timer *timer)
{
	std::vector<uintmax_t> samples = timer->samples();
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
"usage: tack [-vQST] -c [file ...]\n"
"       tack [-vQST] -d [file ...]\n");
	exit(1);
}
