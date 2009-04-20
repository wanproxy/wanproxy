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

#include <xcodec/xcodec.h>

#define	XCDUMP_IOVEC_SIZE	(IOV_MAX)

static void dump(int, int);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void process_files(int, char *[]);
static void usage(void);

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "?")) != -1) {
		switch (ch) {
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	process_files(argc, argv);

	return (0);
}

static void
dump(int ifd, int ofd)
{
	Buffer input, output;

	while (fill(ifd, &input)) {
		BufferSegment *seg;
		uint64_t hash;
		size_t inlen;
		uint8_t ch;

		inlen = input.length();
		while (inlen != 0) {
			ch = input.peek();

			while (!XCODEC_CHAR_SPECIAL(ch)) {
				inlen--;
				input.skip(1);
				output.append("<literal />\n");

				if (inlen == 0)
					break;

				ch = input.peek();
			}

			ASSERT(inlen == input.length());

			if (inlen == 0)
				break;

			switch (ch) {
			case XCODEC_HASHREF_CHAR:
				if (inlen < 9)
					break;

				input.moveout((uint8_t *)&hash, 1, sizeof hash);
				hash = LittleEndian::decode(hash);

				output.append("<hash-reference />\n");
				output.append("<window-declare />\n");
				inlen -= 9;
				continue;

			case XCODEC_ESCAPE_CHAR:
				if (inlen < 2)
					break;
				input.skip(1);
				output.append("<escape />\n");
				input.skip(1);
				inlen -= 2;
				continue;

			case XCODEC_DECLARE_CHAR:
				if (inlen < 9 + XCODEC_SEGMENT_LENGTH)
					break;

				input.moveout((uint8_t *)&hash, 1, sizeof hash);
				hash = LittleEndian::decode(hash);

				input.copyout(&seg, XCODEC_SEGMENT_LENGTH);
				input.skip(XCODEC_SEGMENT_LENGTH);

				output.append("<hash-declare />\n");
				output.append("<window-declare />\n");

				seg->unref();
				inlen -= 9 + XCODEC_SEGMENT_LENGTH;
				continue;

			case XCODEC_BACKREF_CHAR:
				if (inlen < 2)
					break;
				input.skip(1);
				ch = input.peek();
				input.skip(1);

				output.append("<window-reference />\n");

				inlen -= 2;
				continue;

			default:
				NOTREACHED();
			}
			break;
		}

		flush(ofd, &output);
	}
	ASSERT(input.empty());
	ASSERT(output.empty());
}

static bool
fill(int fd, Buffer *input)
{
	BufferSegment *segments[XCDUMP_IOVEC_SIZE];
	struct iovec iov[XCDUMP_IOVEC_SIZE];
	BufferSegment *seg;
	ssize_t len;
	unsigned i;

	for (i = 0; i < XCDUMP_IOVEC_SIZE; i++) {
		seg = new BufferSegment();
		iov[i].iov_base = seg->head();
		iov[i].iov_len = BUFFER_SEGMENT_SIZE;
		segments[i] = seg;
	}

	len = readv(fd, iov, XCDUMP_IOVEC_SIZE);
	if (len == -1)
		HALT("/fill") << "read failed.";
	for (i = 0; i < XCDUMP_IOVEC_SIZE; i++) {
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
process_files(int argc, char *argv[])
{
	int ifd, ofd;

	if (argc == 0) {
		ifd = STDIN_FILENO;
		ofd = STDOUT_FILENO;

		dump(ifd, ofd);
	} else {
		while (argc--) {
			const char *file = *argv++;
			
			ifd = open(file, O_RDONLY);
			ASSERT(ifd != -1);
			ofd = STDOUT_FILENO;

			dump(ifd, ofd);
		}
	}
}

static void
usage(void)
{
	fprintf(stderr,
"usage: xcdump [file ...]\n");
	exit(1);
}
