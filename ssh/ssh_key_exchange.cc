#include <ssh/ssh_key_exchange.h>

namespace {
	class StubKeyExchange : public SSH::KeyExchange {
		LogHandle log_;
	public:
		StubKeyExchange(void)
		: SSH::KeyExchange("diffie-hellman-group-exchange-sha1"),
		  log_("/ssh/key_exchange/stub")
		{ }

		~StubKeyExchange()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}
	};
}

SSH::KeyExchange *
SSH::KeyExchange::method(void)
{
	return (new StubKeyExchange());
}
