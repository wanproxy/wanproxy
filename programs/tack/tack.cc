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

#define	TACK_FLAG_QUIET_OUTPUT		(0x00000001)
#define	TACK_FLAG_CODEC_TIMING		(0x00000002)
#define	TACK_FLAG_BYTE_STATS		(0x00000004)
#define	TACK_FLAG_CODEC_TIMING_EACH	(0x00000008)

static void compress(const std::string&, int, int, XCodec *, unsigned, Timer *);
static void decompress(const std::string&, int, int, XCodec *, unsigned, Timer *);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void print_ratio(const std::string&, uint64_t, uint64_t);
static void process_file(const std::string&, int, int, FileAction, XCodec *, unsigned, Timer *);
static void process_files(int, char *[], FileAction, XCodec *, unsigned, Timer *);
static void time_samples(Timer *);
static void time_stats(const std::string&, Timer *);
static void usage(void);

int
main(int argc, char *argv[])
{
	UUID uuid;
	uuid.generate();

	XCodecCache *cache = XCodecCache::lookup(uuid);
	XCodec codec(cache);

	unsigned flags;
	bool samples, verbose;
	FileAction action;
	int ch;

	action = None;
	samples = false;
	verbose = false;

	while ((ch = getopt(argc, argv, "?cdsvEQST")) != -1) {
		switch (ch) {
		case 'c':
			action = Compress;
			break;
		case 'd':
			action = Decompress;
			break;
		case 's':
			flags |= TACK_FLAG_BYTE_STATS;
			break;
		case 'v':
			verbose = true;
			break;
		case 'E':
			flags |= TACK_FLAG_CODEC_TIMING_EACH;
			break;
		case 'Q':
			flags |= TACK_FLAG_QUIET_OUTPUT;
			break;
		case 'S':
			samples = true;
			break;
		case 'T':
			flags |= TACK_FLAG_CODEC_TIMING;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if ((flags & TACK_FLAG_CODEC_TIMING_EACH) != 0 &&
	    (flags & TACK_FLAG_CODEC_TIMING) == 0)
		usage();

	if (samples && (flags & TACK_FLAG_CODEC_TIMING) == 0)
		usage();

	if ((flags & TACK_FLAG_CODEC_TIMING_EACH) != 0 &&
	    samples)
		usage();

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
		process_files(argc, argv, action, &codec, flags, &codec_timer);

		if ((flags & TACK_FLAG_CODEC_TIMING_EACH) == 0) {
			if (samples)
				time_samples(&codec_timer);
			else
				time_stats("<total>", &codec_timer);
		}
		break;
	default:
		NOTREACHED();
	}

	return (0);
}

static void
compress(const std::string& name, int ifd, int ofd, XCodec *codec, unsigned flags, Timer *timer)
{
	XCodecEncoder encoder(codec->cache());
	Buffer input, output;
	uint64_t inbytes, outbytes;

	if ((flags & TACK_FLAG_BYTE_STATS) != 0)
		inbytes = outbytes = 0;

	while (fill(ifd, &input)) {
		if ((flags & TACK_FLAG_BYTE_STATS) != 0)
			inbytes += input.length();
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0)
			timer->start();
		encoder.encode(&output, &input);
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0)
			timer->stop();
		if ((flags & TACK_FLAG_BYTE_STATS) != 0) {
			inbytes -= input.length();
			outbytes += output.length();
		}
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());

	if ((flags & TACK_FLAG_BYTE_STATS) != 0)
		print_ratio(name, inbytes, outbytes);
}

static void
decompress(const std::string& name, int ifd, int ofd, XCodec *codec, unsigned flags, Timer *timer)
{
	std::set<uint64_t> unknown_hashes;
	XCodecDecoder decoder(codec->cache());
	Buffer input, output;
	uint64_t inbytes, outbytes;

	if ((flags & TACK_FLAG_BYTE_STATS) != 0)
		inbytes = outbytes = 0;

	while (fill(ifd, &input)) {
		if ((flags & TACK_FLAG_BYTE_STATS) != 0)
			inbytes += input.length();
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0)
			timer->start();
		if (!decoder.decode(&output, &input, unknown_hashes)) {
			ERROR("/decompress") << "Decode failed.";
			return;
		}
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0)
			timer->stop();
		if (!unknown_hashes.empty()) {
			ERROR("/decompress") << "Cannot decode stream with unknown hashes.";
			return;
		}
		if ((flags & TACK_FLAG_BYTE_STATS) != 0) {
			inbytes -= input.length();
			outbytes += output.length();
		}
		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());

	if ((flags & TACK_FLAG_BYTE_STATS) != 0)
		print_ratio(name, outbytes, inbytes); /* Reverse order of compress().  */
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
	INFO("/codec_stats") << name << ": " << inbytes << " uncompressed bytes, " << outbytes << " compressed bytes.";
	if (inbytes <= outbytes) {
		INFO("/codec_stats") << name << ": bloat ratio 1:" << ((float)outbytes / inbytes) << " (" << ((float)inbytes / outbytes) << ":1)";
	} else {
		INFO("/codec_stats") << name << ": compression ratio 1:" << ((float)outbytes / inbytes) << " (" << ((float)inbytes / outbytes) << ":1)";
	}
}

static void
process_file(const std::string& name, int ifd, int ofd, FileAction action, XCodec *codec, unsigned flags, Timer *timer)
{
	switch (action) {
	case Compress:
		compress(name, ifd, ofd, codec, flags, timer);
		break;
	case Decompress:
		decompress(name, ifd, ofd, codec, flags, timer);
		break;
	default:
		NOTREACHED();
	}

	if ((flags & TACK_FLAG_CODEC_TIMING_EACH) != 0)
		time_stats(name, timer);
}

static void
process_files(int argc, char *argv[], FileAction action, XCodec *codec, unsigned flags, Timer *timer)
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		if ((flags & TACK_FLAG_QUIET_OUTPUT) != 0)
			ofd = -1;
		else
			ofd = STDOUT_FILENO;
		process_file("<stdin>", ifd, ofd, action, codec, flags, timer);
	} else {
		while (argc--) {
			const char *file = *argv++;

			ifd = open(file, O_RDONLY);
			if (ifd == -1) {
				ERROR("/tack") << "Could not open: " << file;
				continue;
			}

			if ((flags & TACK_FLAG_QUIET_OUTPUT) != 0)
				ofd = -1;
			else
				ofd = STDOUT_FILENO;

			process_file(file, ifd, ofd, action, codec, flags, timer);

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
time_stats(const std::string& name, Timer *timer)
{
	std::vector<uintmax_t> samples = timer->samples();
	std::vector<uintmax_t>::iterator it;
	LogHandle log("/codec_timer");
	uintmax_t microseconds;

	INFO(log) << name << ": " << samples.size() << " timer samples.";
	microseconds = 0;
	for (it = samples.begin(); it != samples.end(); ++it)
		microseconds += *it;
	INFO(log) << name << ": " << microseconds << " total runtime.";
	INFO(log) << name << ": " << (microseconds / samples.size()) << " mean microseconds/call.";

	timer->reset();
}

static void
usage(void)
{
	fprintf(stderr,
"usage: tack [-svQ] [-T [-E | -S]] -c [file ...]\n"
"       tack [-svQ] [-T [-E | -S]] -d [file ...]\n");
	exit(1);
}
