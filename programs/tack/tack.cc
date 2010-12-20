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

static bool codec_timing;
static Timer codec_timer;

static void compress(int, int, XCodec *);
static void decompress(int, int, XCodec *);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void process_files(int, char *[], FileAction, XCodec *, bool);
static void time_samples(void);
static void time_stats(void);
static void usage(void);

int
main(int argc, char *argv[])
{
	UUID uuid;
	uuid.generate();

	XCodecCache *cache = XCodecCache::lookup(uuid);
	XCodec codec(cache);

	bool quiet_output, samples, verbose;
	FileAction action;
	int ch;

	action = None;
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

	switch (action) {
	case None:
		usage();
	case Compress:
	case Decompress:
		process_files(argc, argv, action, &codec, quiet_output);
		if (samples)
			time_samples();
		if (codec_timing)
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
	XCodecEncoder encoder(codec->cache());
	Buffer input, output;

	while (fill(ifd, &input)) {
		if (codec_timing)
			codec_timer.start();
		encoder.encode(&output, &input);
		if (codec_timing)
			codec_timer.stop();
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static void
decompress(int ifd, int ofd, XCodec *codec)
{
	XCodecDecoder decoder(codec->cache());
	Buffer input, output;

	(void)codec;

	while (fill(ifd, &input)) {
		if (codec_timing)
			codec_timer.start();
		if (!decoder.decode(&output, &input)) {
			ERROR("/decompress") << "Decode failed.";
			return;
		}
		if (codec_timing)
			codec_timer.stop();
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
process_files(int argc, char *argv[], FileAction action, XCodec *codec, bool quiet)
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		if (quiet)
			ofd = -1;
		else
			ofd = STDOUT_FILENO;
		process_file(ifd, ofd, action, codec);
	} else {
		while (argc--) {
			const char *file = *argv++;

			ifd = open(file, O_RDONLY);
			ASSERT(ifd != -1);
			if (quiet)
				ofd = -1;
			else
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
"usage: tack [-vQST] -c [file ...]\n"
"       tack [-vQST] -d [file ...]\n");
	exit(1);
}
