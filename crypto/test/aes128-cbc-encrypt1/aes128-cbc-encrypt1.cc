/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#include <common/test.h>

#include <crypto/crypto_encryption.h>

#include <event/event_main.h>

class CryptoTest {
	TestGroup& group_;
	CryptoEncryption::Session *session_;
	Test callback_called_;
	Test destructor_called_;
	Action *action_;
	Buffer ciphertext_;
public:
	CryptoTest(TestGroup &group, CryptoEncryption::Cipher cipher, const Buffer& key, const Buffer& iv, const Buffer& plaintext, const Buffer& ciphertext)
	: group_(group),
	  session_(NULL),
	  callback_called_(group, "Callback called."),
	  destructor_called_(group, "Destructor called."),
	  action_(NULL),
	  ciphertext_(ciphertext)
	{
		Buffer tmp(plaintext);

		const CryptoEncryption::Method *method = CryptoEncryption::Method::method(cipher);
		if (method == NULL)
			HALT("/test/crypto/aes128-cbc/encrypt1") << "Could not find a suitable method.";

		session_ = method->session(cipher);
		{
			Test _(group_, "Session initialize.");
			if (session_->initialize(CryptoEncryption::Encrypt, &key, &iv))
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

	CryptoTest _(g, CryptoEncryption::Cipher(CryptoEncryption::AES128, CryptoEncryption::CBC), key, iv, plaintext, ciphertext);

	event_main();
}
