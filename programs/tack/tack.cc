#include <sys/types.h>
#include <sys/errno.h>
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

static void compress(const std::string&, int, int, XCodec *, bool, Timer *, bool);
static void decompress(const std::string&, int, int, XCodec *, bool, Timer *, bool);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void print_ratio(const std::string&, uint64_t, uint64_t);
static void process_file(const std::string&, int, int, FileAction, XCodec *, bool, Timer *, bool);
static void process_files(int, char *[], FileAction, XCodec *, bool, bool, Timer *, bool);
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

	bool codec_timing, quiet_output, samples, stats, verbose;
	FileAction action;
	int ch;

	action = None;
	codec_timing = false;
	quiet_output = false;
	samples = false;
	stats = false;
	verbose = false;

	while ((ch = getopt(argc, argv, "?cdsvQST")) != -1) {
		switch (ch) {
		case 'c':
			action = Compress;
			break;
		case 'd':
			action = Decompress;
			break;
		case 's':
			stats = true;
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
		process_files(argc, argv, action, &codec, quiet_output, codec_timing, &codec_timer, stats);
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
compress(const std::string& name, int ifd, int ofd, XCodec *codec, bool timing, Timer *timer, bool stats)
{
	XCodecEncoder encoder(codec->cache());
	Buffer input, output;
	uint64_t inbytes, outbytes;

	if (stats)
		inbytes = outbytes = 0;

	while (fill(ifd, &input)) {
		if (stats)
			inbytes += input.length();
		if (timing)
			timer->start();
		encoder.encode(&output, &input);
		if (timing)
			timer->stop();
		if (stats) {
			inbytes -= input.length();
			outbytes += output.length();
		}
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());

	if (stats)
		print_ratio(name, inbytes, outbytes);
}

static void
decompress(const std::string& name, int ifd, int ofd, XCodec *codec, bool timing, Timer *timer, bool stats)
{
	std::set<uint64_t> unknown_hashes;
	XCodecDecoder decoder(codec->cache());
	Buffer input, output;
	uint64_t inbytes, outbytes;

	(void)codec;

	if (stats)
		inbytes = outbytes = 0;

	while (fill(ifd, &input)) {
		if (stats)
			inbytes += input.length();
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
		if (stats) {
			inbytes -= input.length();
			outbytes += output.length();
		}
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());

	if (stats)
		print_ratio(name, inbytes, outbytes);
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
print_ratio(const std::string& name, uint64_t inbytes, uint64_t outbytes)
{
	INFO("/codec_stats") << name << ": " << inbytes << " bytes in, " << outbytes << " bytes out.";
	if (inbytes <= outbytes) {
		INFO("/codec_stats") << name << ": bloat ratio 1:" << ((float)outbytes / inbytes) << " (" << ((float)inbytes / outbytes) << ":1)";
	} else {
		INFO("/codec_stats") << name << ": compression ratio 1:" << ((float)outbytes / inbytes) << " (" << ((float)inbytes / outbytes) << ":1)";
	}
}

static void
process_file(const std::string& name, int ifd, int ofd, FileAction action, XCodec *codec, bool timing, Timer *timer, bool stats)
{
	switch (action) {
	case Compress:
		compress(name, ifd, ofd, codec, timing, timer, stats);
		break;
	case Decompress:
		decompress(name, ifd, ofd, codec, timing, timer, stats);
		break;
	default:
		NOTREACHED();
	}
}

static void
process_files(int argc, char *argv[], FileAction action, XCodec *codec, bool quiet, bool timing, Timer *timer, bool stats)
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		if (quiet)
			ofd = -1;
		else
			ofd = STDOUT_FILENO;
		process_file("<stdin>", ifd, ofd, action, codec, timing, timer, stats);
	} else {
		while (argc--) {
			const char *file = *argv++;

			ifd = open(file, O_RDONLY);
			ASSERT(ifd != -1);
			if (quiet)
				ofd = -1;
			else
				ofd = STDOUT_FILENO;

			process_file(file, ifd, ofd, action, codec, timing, timer, stats);

			close(ifd);
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
"usage: tack [-svQST] -c [file ...]\n"
"       tack [-svQST] -d [file ...]\n");
	exit(1);
}
