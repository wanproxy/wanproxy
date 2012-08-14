#include <ssh/ssh_server_host_key.h>

namespace {
	class FakeServerHostKey : public SSH::ServerHostKey {
		LogHandle log_;
	public:
		FakeServerHostKey(void)
		: SSH::ServerHostKey("ssh-rsa"),
		  log_("/ssh/compression/fake")
		{ }

		~FakeServerHostKey()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}
	};
}

SSH::ServerHostKey *
SSH::ServerHostKey::server(const std::string& keyfile)
{
	(void)keyfile;
	return (new FakeServerHostKey());
}
