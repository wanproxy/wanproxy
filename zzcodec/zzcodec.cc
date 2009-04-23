#include <common/buffer.h>

#include <zzcodec/zzcodec.h>

#define	ZZCODEC_CHAR_BASE	((uint8_t)0xf8)
#define	ZZCODEC_CHAR(option)	(ZZCODEC_CHAR_BASE | (option))
#define	ZZCODEC_BLOCK_CHAR	(ZZCODEC_CHAR(0x00))
#define	ZZCODEC_ESCAPE_CHAR	(ZZCODEC_CHAR(0x01))
#define	ZZCODEC_CHAR_MASK	(0x01)
#define	ZZCODEC_CHAR_SPECIAL(ch)	(((ch) & ~ZZCODEC_CHAR_MASK) == ZZCODEC_CHAR_BASE)

struct zzcodec_special_p {
	bool operator() (uint8_t ch) const
	{
		return (ZZCODEC_CHAR_SPECIAL(ch));
	}
};

ZZCodec::ZZCodec(void)
: log_("/zzcodec"),
  transform_()
{
	unsigned m, x, y;

	m = 0;
	x = 0;
	y = 0;

	transform_[m++] = y;

	while (m < ZZCODEC_COLUMNS * ZZCODEC_ROWS) {
		while (y + 1 < ZZCODEC_ROWS && x != 0) {
			y++;
			x--;

			transform_[m++] = y;
		}

		if (y + 1 == ZZCODEC_ROWS) {
			x++;
		} else {
			y++;
		}
		transform_[m++] = y;

		while (x + 1 < ZZCODEC_COLUMNS && y != 0) {
			y--;
			x++;

			transform_[m++] = y;
		}

		if (x + 1 == ZZCODEC_COLUMNS) {
			y++;
		} else {
			x++;
		}
		transform_[m++] = y;
	}
}

bool
ZZCodec::decode(Buffer *output, Buffer *input) const
{
	size_t inlen;
	uint8_t ch;

	inlen = input->length();
	while (inlen != 0) {
		ch = input->peek();

		while (!ZZCODEC_CHAR_SPECIAL(ch)) {
			inlen--;
			input->skip(1);
			output->append(ch);

			if (inlen == 0)
				break;

			ch = input->peek();
		}

		ASSERT(inlen == input->length());

		if (inlen == 0)
			break;

		Buffer rows[ZZCODEC_ROWS];
		unsigned i;

		switch (ch) {
			/*
			 * A block that has been transformed by zig-zagging.
			 */
		case ZZCODEC_BLOCK_CHAR:
			if (inlen < 1 + (ZZCODEC_COLUMNS * ZZCODEC_ROWS))
				return (true);

			input->skip(1);

			for (i = 0; i < ZZCODEC_COLUMNS * ZZCODEC_ROWS; i++) {
				Buffer *row = &rows[transform_[i]];

				row->append(input->peek());
				input->skip(1);
			}

			for (i = 0; i < ZZCODEC_ROWS; i++) {
				Buffer *row = &rows[i];

				ASSERT(row->length() == ZZCODEC_COLUMNS);
				output->append(row);
				row->clear();
			}

			inlen -= 1 + (ZZCODEC_COLUMNS * ZZCODEC_ROWS);
			break;

			/*
			 * A literal character will follow that would
			 * otherwise have seemed magic.
			 */
		case ZZCODEC_ESCAPE_CHAR:
			if (inlen < 2)
				return (true);
			input->skip(1);
			output->append(input->peek());
			input->skip(1);
			inlen -= 2;
			break;

		default:
			NOTREACHED();
		}
	}

	return (true);

}

void
ZZCodec::encode(Buffer *output, Buffer *input) const
{
	while (input->length() >= ZZCODEC_COLUMNS * ZZCODEC_ROWS) {
		Buffer rows[ZZCODEC_ROWS];
		unsigned i;

		/*
		 * Could just copyout, but I suspect this is faster.
		 */
		for (i = 0; i < ZZCODEC_ROWS; i++) {
			Buffer *row = &rows[i];

			input->moveout(row, ZZCODEC_COLUMNS);
		}

		output->append(ZZCODEC_BLOCK_CHAR);

		for (i = 0; i < ZZCODEC_COLUMNS * ZZCODEC_ROWS; i++) {
			Buffer *row = &rows[transform_[i]];

			output->append(row->peek());
			row->skip(1);
		}

#ifndef NDEBUG
		for (i = 0; i < ZZCODEC_ROWS; i++) {
			Buffer *row = &rows[i];

			ASSERT(row->empty());
		}
#endif
	}

	if (!input->empty()) {
		input->escape(ZZCODEC_ESCAPE_CHAR, zzcodec_special_p());
		output->append(input);
		input->clear();
	}
}
