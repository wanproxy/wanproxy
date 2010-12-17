#include <common/test.h>

#include <crypto/crypto_encryption.h>

#include <event/event_main.h>

class CryptoTest {
	TestGroup& group_;
	CryptoEncryptionSession *session_;
	Test callback_called_;
	Test destructor_called_;
	Action *action_;
	Buffer ciphertext_;
public:
	CryptoTest(TestGroup &group, CryptoCipher cipher, const Buffer& key, const Buffer& iv, const Buffer& plaintext, const Buffer& ciphertext)
	: group_(group),
	  session_(NULL),
	  callback_called_(group, "Callback called."),
	  destructor_called_(group, "Destructor called."),
	  action_(NULL),
	  ciphertext_(ciphertext)
	{
		Buffer tmp(plaintext);

		session_ = CryptoEncryptionMethod::default_method->session(cipher);
		{
			Test _(group_, "Session initialize.");
			if (session_->initialize(CryptoEncrypt, &key, &iv))
				_.pass();
		}
		EventCallback *cb = callback(this, &CryptoTest::session_callback);
		action_ = session_->submit(&tmp, cb);
	}

	~CryptoTest()
	{
		destructor_called_.pass();
	}

	void session_callback(Event e)
	{
		callback_called_.pass();

		{
			Test _(group_, "Outstanding action.");
			if (action_ != NULL)
				_.pass();
		}
		action_->cancel();
		action_ = NULL;

		{
			Test _(group_, "Encryption done.");
			if (e.type_ == Event::Done)
				_.pass();
		}

		{
			Test _(group_, "Expected ciphertext.");
			if (e.buffer_.equal(&ciphertext_))
				_.pass();
		}
	}
};

int
main(void)
{
	TestGroup g("/test/crypto/aes128-cbc/encrypt1", "AES128-CBC Encrypt #1");

	Buffer key("\x6c\x3e\xa0\x47\x76\x30\xce\x21\xa2\xce\x33\x4a\xa7\x46\xc2\xcd");
	Buffer iv("\xc7\x82\xdc\x4c\x09\x8c\x66\xcb\xd9\xcd\x27\xd8\x25\x68\x2c\x81");
	Buffer plaintext("This is a 48-byte message (exactly 3 AES blocks)");
	Buffer ciphertext("\xd0\xa0\x2b\x38\x36\x45\x17\x53\xd4\x93\x66\x5d\x33\xf0\xe8\x86\x2d\xea\x54\xcd\xb2\x93\xab\xc7\x50\x69\x39\x27\x67\x72\xf8\xd5\x02\x1c\x19\x21\x6b\xad\x52\x5c\x85\x79\x69\x5d\x83\xba\x26\x84");

	CryptoTest _(g, CryptoCipher(CryptoAES128, CryptoModeCBC), key, iv, plaintext, ciphertext);

	event_main();
}
