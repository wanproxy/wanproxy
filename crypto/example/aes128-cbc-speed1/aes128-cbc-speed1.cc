#include <crypto/crypto_encryption.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

static uint8_t zbuf[8192];

class CryptoSpeed {
	CryptoEncryptionSession *session_;
	Buffer buffer_;
	uintmax_t bytes_;
	Action *callback_action_;
	Action *timeout_action_;
public:
	CryptoSpeed(CryptoCipher cipher, const Buffer& key, const Buffer& iv)
	: session_(NULL),
	  buffer_(zbuf, sizeof zbuf),
	  bytes_(0),
	  callback_action_(NULL),
	  timeout_action_(NULL)
	{
		session_ = CryptoEncryptionMethod::default_method->session(cipher);
		if (!session_->initialize(CryptoEncrypt, &key, &iv))
			HALT("/example/aes128-cbc/speed1") << "Failed to initialize session.";

		callback_action_ = callback(this, &CryptoSpeed::callback_complete)->schedule();

		INFO("/example/aes128-cbc/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &CryptoSpeed::timer));
	}

	~CryptoSpeed()
	{
		ASSERT(timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		Buffer tmp(buffer_);

		EventCallback *cb = callback(this, &CryptoSpeed::session_callback);
		callback_action_ = session_->submit(&tmp, cb);
	}

	void session_callback(Event e)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		ASSERT(e.type_ == Event::Done);
		bytes_ += e.buffer_.length();

		callback_action_ = callback(this, &CryptoSpeed::callback_complete)->schedule();
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		ASSERT(callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;

		INFO("/example/aes128-cbc/speed1") << "Timer expired; " << bytes_ << " bytes appended.";
	}
};

int
main(void)
{
	INFO("/example/aes128-cbc/speed1") << "Timer delay: " << TIMER_MS << "ms";

	memset(zbuf, 0, sizeof zbuf);

	Buffer key("\x6c\x3e\xa0\x47\x76\x30\xce\x21\xa2\xce\x33\x4a\xa7\x46\xc2\xcd");
	Buffer iv("\xc7\x82\xdc\x4c\x09\x8c\x66\xcb\xd9\xcd\x27\xd8\x25\x68\x2c\x81");

	CryptoSpeed _(CryptoCipher(CryptoAES128, CryptoModeCBC), key, iv);

	event_main();
}
