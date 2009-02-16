#include <sys/uio.h>
#include <limits.h>

#include <iostream>
#include <string>

#include <common/debug.h>
#include <common/log.h>

#include <common/buffer.h>

struct string_builder {
	std::string string_;

	string_builder(void)
	: string_()
	{ }

	void operator() (const BufferSegment *seg)
	{
		std::string str((const char *)seg->data(), seg->length());
		string_ += str;
	}
};

size_t
Buffer::fill_iovec(struct iovec *iov, size_t niov) const
{
	if (niov == 0)
		return (0);

	if (niov > IOV_MAX)
		niov = IOV_MAX;

	segment_list_t::const_iterator iter = data_.begin();
	size_t iovcnt = 0;
	unsigned i;

	for (i = 0; i < niov; i++) {
		if (iter == data_.end())
			break;
		const BufferSegment *seg = *iter;
		iov[i].iov_base = (void *)(uintptr_t)seg->data();
		iov[i].iov_len = seg->length();
		iovcnt++;
		iter++;
	}
	return (iovcnt);
}

std::ostream&
operator<< (std::ostream& os, const Buffer *buf)
{
	string_builder sb;
	buf->foreach_segment(sb);
	return (os << sb.string_);
}

std::ostream&
operator<< (std::ostream& os, const Buffer& buf)
{
	return (os << &buf);
}
