/*
 * Copyright (c) 2008-2014 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
#include <xcodec/xcodec_hash.h>

enum FileAction {
	None, Compress, Decompress, Hashes
};

#define	TACK_FLAG_QUIET_OUTPUT		(0x00000001)
#define	TACK_FLAG_CODEC_TIMING		(0x00000002)
#define	TACK_FLAG_BYTE_STATS		(0x00000004)
#define	TACK_FLAG_CODEC_TIMING_EACH	(0x00000008)
#define	TACK_FLAG_CODEC_TIMING_SAMPLES	(0x00000010)

static void compress(const std::string&, int, int, XCodec *, unsigned, Timer *);
static void decompress(const std::string&, int, int, XCodec *, unsigned, Timer *);
static void hashes(int, int, unsigned, Timer *);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void print_ratio(const std::string&, uint64_t, uint64_t);
static void process_file(const std::string&, int, int, FileAction, XCodec *, unsigned, Timer *);
static void process_files(int, char *[], FileAction, XCodec *, unsigned);
static void time_samples(const std::string&, Timer *);
static void time_stats(const std::string&, Timer *);
static void usage(void);

class TackNullCache : public XCodecCache {
public:
	TackNullCache(const UUID& uuid)
	: XCodecCache(uuid)
	{ }

	~TackNullCache()
	{ }

	BufferSegment *lookup(const uint64_t&)
	{
		return (NULL);
	}

	void enter(const uint64_t&, BufferSegment *)
	{ }

	bool out_of_band(void) const
	{
		return (true);
	}
};

class TackPersistentCache : public XCodecCache {
	int fd_;
	XCodecCache *cache_;
	Buffer new_data_;
public:
	TackPersistentCache(const UUID& uuid, int fd)
	: XCodecCache(uuid),
	  fd_(fd),
	  cache_(new XCodecMemoryCache(uuid)),
	  new_data_()
	{
		Buffer input;
		while (fill(fd_, &input)) {
			while (!input.empty()) {
				BufferSegment *seg;
				input.copyout(&seg, XCODEC_SEGMENT_LENGTH);
				if (seg->length() != XCODEC_SEGMENT_LENGTH)
					HALT("/tack/persistent/cache") << "Corrupt persistent cache.";
				input.skip(XCODEC_SEGMENT_LENGTH);

				uint64_t hash = XCodecHash::hash(seg->data());
				cache_->enter(hash, seg);
			}
		}
	}

	~TackPersistentCache()
	{
		delete cache_;
		cache_ = NULL;

		if (!new_data_.empty())
			flush(fd_, &new_data_);
		close(fd_);
		fd_ = -1;
	}

	BufferSegment *lookup(const uint64_t& hash)
	{
		return (cache_->lookup(hash));
	}

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		cache_->enter(hash, seg);
		new_data_.append(seg);
	}

	bool out_of_band(void) const
	{
		return (true);
	}
};

int
main(int argc, char *argv[])
{
	const char *persist;
	bool nullcache;
	bool verbose;
	FileAction action;
	unsigned flags;
	int ch;

	persist = NULL;
	action = None;
	flags = 0;
	nullcache = false;
	verbose = false;

	while ((ch = getopt(argc, argv, "?cdhp:svENQST")) != -1) {
		switch (ch) {
		case 'c':
			action = Compress;
			break;
		case 'd':
			action = Decompress;
			break;
		case 'h':
			action = Hashes;
			break;
		case 'p':
			persist = optarg;
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
		case 'N':
			nullcache = true;
			break;
		case 'Q':
			flags |= TACK_FLAG_QUIET_OUTPUT;
			break;
		case 'S':
			flags |= TACK_FLAG_CODEC_TIMING_SAMPLES;
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

	if (action == None)
		usage();

	if (action == Hashes) {
		if ((flags & TACK_FLAG_BYTE_STATS) != 0)
			usage();
		if (nullcache)
			usage();
		if (persist != NULL)
			usage();
	}

	if (persist != NULL && nullcache)
		usage();

	if ((flags & TACK_FLAG_CODEC_TIMING) == 0 &&
	    (flags & (TACK_FLAG_CODEC_TIMING_EACH | TACK_FLAG_CODEC_TIMING_SAMPLES)) != 0)
		usage();

	if (verbose) {
		Log::mask(".?", Log::Debug);
	} else {
		Log::mask(".?", Log::Info);
	}

	UUID uuid;
	uuid.generate();

	XCodecCache *cache;
	if (persist == NULL) {
		if (nullcache)
			cache = new TackNullCache(uuid);
		else
			cache = new XCodecMemoryCache(uuid);
	} else {
		int fd;
		if (action == Compress) {
			fd = open(persist, O_RDWR | O_CREAT, 0600);
		} else {
			/*
			 * When decompressing, only allow reads.
			 */
			fd = open(persist, O_RDONLY);
		}
		if (fd == -1)
			HALT("/tack") << "Could not open persistent cache.";
		cache = new TackPersistentCache(uuid, fd);
	}
	XCodec codec(cache);

	process_files(argc, argv, action, &codec, flags);

	delete cache;

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
	ASSERT("/compress", input.empty());
	ASSERT("/compress", output.empty());

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
	ASSERT("/decompress", input.empty());
	ASSERT("/decompress", output.empty());

	if ((flags & TACK_FLAG_BYTE_STATS) != 0)
		print_ratio(name, outbytes, inbytes); /* Reverse order of compress().  */
}

static void
hashes(int ifd, int ofd, unsigned flags, Timer *timer)
{
	Buffer input, output;
	BufferSegment *seg;
	unsigned o;

	while (fill(ifd, &input)) {
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0)
			timer->start();
		XCodecHash xcodec_hash;
		o = 0;
		while (!input.empty()) {
			input.moveout(&seg);

			const uint8_t *p = seg->data();
			const uint8_t *q = seg->end();
			for (;;) {
				while (o < XCODEC_SEGMENT_LENGTH && p != q) {
					xcodec_hash.add(*p++);
					o++;
				}
				if (o == XCODEC_SEGMENT_LENGTH) {
					uint64_t hash = xcodec_hash.mix();
					xcodec_hash.reset();
					hash = BigEndian::encode(hash);
					output.append(&hash);
					o = 0;

					if (p == q)
						break;

					continue;
				}
				ASSERT("/hashes", p == q);
				break;
			}

			seg->unref();
		}
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0)
			timer->stop();
		flush(ofd, &output);
	}
	ASSERT("/hashes", input.empty());
	ASSERT("/hashes", output.empty());
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
	ASSERT("/flush", output->empty());
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
	if ((flags & TACK_FLAG_CODEC_TIMING_EACH) != 0) {
		ASSERT("/process/file", timer == NULL);

		timer = new Timer();
	}

	switch (action) {
	case Compress:
		compress(name, ifd, ofd, codec, flags, timer);
		break;
	case Decompress:
		decompress(name, ifd, ofd, codec, flags, timer);
		break;
	case Hashes:
		hashes(ifd, ofd, flags, timer);
		break;
	default:
		NOTREACHED("/process/file");
	}

	if ((flags & TACK_FLAG_CODEC_TIMING_EACH) != 0) {
		if ((flags & TACK_FLAG_CODEC_TIMING_SAMPLES) != 0)
			time_samples(name, timer);
		else
			time_stats(name, timer);
		delete timer;
	}
}

static void
process_files(int argc, char *argv[], FileAction action, XCodec *codec, unsigned flags)
{
	Timer *timer;
	int ifd, ofd;
	bool opened;

	if ((flags & TACK_FLAG_CODEC_TIMING) != 0 &&
	    (flags & TACK_FLAG_CODEC_TIMING_EACH) == 0)
		timer = new Timer();
	else
		timer = NULL;

	if (argc == 0) {
		opened = true;

		ifd = STDIN_FILENO;
		if ((flags & TACK_FLAG_QUIET_OUTPUT) != 0)
			ofd = -1;
		else
			ofd = STDOUT_FILENO;
		process_file("<stdin>", ifd, ofd, action, codec, flags, timer);
	} else {
		opened = false;

		while (argc--) {
			const char *file = *argv++;

			ifd = open(file, O_RDONLY);
			if (ifd == -1) {
				ERROR("/tack") << "Could not open: " << file;
				continue;
			}

			opened = true;

			if ((flags & TACK_FLAG_QUIET_OUTPUT) != 0)
				ofd = -1;
			else
				ofd = STDOUT_FILENO;

			process_file(file, ifd, ofd, action, codec, flags, timer);

			close(ifd);
		}
	}

	if (opened) {
		if ((flags & TACK_FLAG_CODEC_TIMING) != 0 &&
		    (flags & TACK_FLAG_CODEC_TIMING_EACH) == 0) {
			ASSERT("/process/files", timer != NULL);
			if ((flags & TACK_FLAG_CODEC_TIMING_SAMPLES) != 0)
				time_samples("", timer);
			else
				time_stats("<total>", timer);
			delete timer;
		}
	}
}

static void
time_samples(const std::string& name, Timer *timer)
{
	std::vector<uintmax_t> samples = timer->samples();
	std::vector<uintmax_t>::iterator it;

	if (name == "") {
		for (it = samples.begin(); it != samples.end(); ++it)
			fprintf(stderr, "%ju\n", *it);
	} else {
		for (it = samples.begin(); it != samples.end(); ++it)
			fprintf(stderr, "%s,%ju\n", name.c_str(), *it);
	}
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
}

static void
usage(void)
{
	fprintf(stderr,
"usage: tack [-p cache | -N] [-svQ] [-T [-ES]] -c [file ...]\n"
"       tack [-p cache | -N] [-svQ] [-T [-ES]] -d [file ...]\n"
"       tack [-vQ] [-T [-ES]] -h [file ...]\n");
	exit(1);
}
