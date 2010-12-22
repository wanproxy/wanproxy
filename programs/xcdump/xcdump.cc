#include <sys/types.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include <map>
#include <vector>

#include <common/buffer.h>
#include <common/endian.h>
#include <common/limits.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_hash.h>

#define	XCDUMP_IOVEC_SIZE	(IOV_MAX)

static int dump_verbosity;

static void bhexdump(Buffer *, const uint8_t *, size_t);
static void bprintf(Buffer *, const char *, ...);
static void dump(int, int);
static bool fill(int, Buffer *);
static void flush(int, Buffer *);
static void process_files(int, char *[]);
static void usage(void);

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "?v")) != -1) {
		switch (ch) {
		case 'v':
			dump_verbosity++;
			break;
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
bhexdump(Buffer *output, const uint8_t *data, size_t len)
{
	while (len--)
		bprintf(output, "%02x", *data++);
}

static void
bprintf(Buffer *output, const char *fmt, ...)
{
	char buf[65536];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	output->append(std::string(buf));
}

static void
dump(int ifd, int ofd)
{
	Buffer input, output;

	while (fill(ifd, &input)) {
		while (!input.empty()) {
			if (input.empty())
				break;

			unsigned off;
			if (!input.find(XCODEC_MAGIC, &off)) {
				bprintf(&output, "<data");
				if (dump_verbosity > 1) {
					uint8_t data[input.length()];
					input.copyout(data, sizeof data);

					bprintf(&output, " data=\"");
					bhexdump(&output, data, sizeof data);
					bprintf(&output, "\"");
				}
				input.clear();
				bprintf(&output, "/>\n");
				break;
			}

			if (off != 0) {
				bprintf(&output, "<data");
				if (dump_verbosity > 1) {
					uint8_t data[off];
					input.copyout(data, sizeof data);

					bprintf(&output, " data=\"");
					bhexdump(&output, data, sizeof data);
					bprintf(&output, "\"");
				}
				input.skip(off);
				bprintf(&output, "/>\n");
			}

			/*
			 * Need the following byte at least.
			 */
			if (input.length() == 1)
				break;

			uint8_t op;
			input.copyout(&op, sizeof XCODEC_MAGIC, sizeof op);
			switch (op) {
			case XCODEC_OP_ESCAPE:
				bprintf(&output, "<escape />");
				input.skip(sizeof XCODEC_MAGIC + sizeof op);
				continue;
			case XCODEC_OP_EXTRACT:
				if (input.length() < sizeof XCODEC_MAGIC + sizeof op + XCODEC_SEGMENT_LENGTH)
					break;
				else {
					input.skip(sizeof XCODEC_MAGIC + sizeof op);

					BufferSegment *seg;
					input.copyout(&seg, XCODEC_SEGMENT_LENGTH);
					input.skip(XCODEC_SEGMENT_LENGTH);

					uint64_t hash = XCodecHash::hash(seg->data());

					bprintf(&output, "<hash-declare");
					if (dump_verbosity > 0) {
						bprintf(&output, " hash=\"0x%016jx\"", (uintmax_t)hash);
						if (dump_verbosity > 1) {
							bprintf(&output, " data=\"");
							bhexdump(&output, seg->data(), seg->length());
							bprintf(&output, "\"");
						}
					}
					bprintf(&output, "/>\n");

					seg->unref();
				}
				continue;
			case XCODEC_OP_REF:
				if (input.length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint64_t))
					break;
				else {
					uint64_t behash;
					input.moveout((uint8_t *)&behash, sizeof XCODEC_MAGIC + sizeof op, sizeof behash);
					uint64_t hash = BigEndian::decode(behash);

					bprintf(&output, "<hash-reference");
					if (dump_verbosity > 0)
						bprintf(&output, " hash=\"0x%016jx\"", (uintmax_t)hash);
					bprintf(&output, "/>\n");
				}
				continue;
			case XCODEC_OP_BACKREF:
				if (input.length() < sizeof XCODEC_MAGIC + sizeof op + sizeof (uint8_t))
					break;
				else {
					uint8_t idx;
					input.moveout(&idx, sizeof XCODEC_MAGIC + sizeof op, sizeof idx);

					bprintf(&output, "<back-reference");
					if (dump_verbosity > 0)
						bprintf(&output, " offset=\"%u\"", (unsigned)idx);
					bprintf(&output, "/>\n");
				}
				continue;
			default:
				ERROR("/dump") << "Unsupported XCodec opcode " << (unsigned)op << ".";
				return;
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
"usage: xcdump [-v] [file ...]\n");
	exit(1);
}
