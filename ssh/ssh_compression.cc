#include <ssh/ssh_compression.h>

namespace {
	class NoneCompression : public SSH::Compression {
		LogHandle log_;
	public:
		NoneCompression(void)
		: SSH::Compression("none"),
		  log_("/ssh/compression/none")
		{ }

		~NoneCompression()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}
	};
}

SSH::Compression *
SSH::Compression::none(void)
{
	return (new NoneCompression());
}
