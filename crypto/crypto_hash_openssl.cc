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

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <common/factory.h>

#include <crypto/crypto_hash.h>

namespace {
	class InstanceEVP : public CryptoHash::Instance {
		LogHandle log_;
		const EVP_MD *algorithm_;
	public:
		InstanceEVP(const EVP_MD *algorithm)
		: log_("/crypto/hash/instance/openssl"),
		  algorithm_(algorithm)
		{ }

		~InstanceEVP()
		{ }

		virtual bool hash(Buffer *out, const Buffer *in)
		{
			/*
			 * We process a single, large, linear byte buffer here rather
			 * than going a BufferSegment at a time, even though the byte
			 * buffer is less efficient than some alternatives, because
			 * there are padding and buffering implications if each
			 * BufferSegment's length is not modular to the block size.
			 */
			uint8_t indata[in->length()];
			in->copyout(indata, sizeof indata);

			uint8_t macdata[EVP_MD_size(algorithm_)];
			unsigned maclen;
			if (!EVP_Digest(indata, sizeof indata, macdata, &maclen, algorithm_, NULL))
				return (false);
			ASSERT(log_, maclen == sizeof macdata);
			out->append(macdata, maclen);
			return (true);
		}

		Action *submit(Buffer *in, EventCallback *cb)
		{
			Buffer out;
			if (!hash(&out, in)) {
				in->clear();
				cb->param(Event::Error);
				return (cb->schedule());
			}
			in->clear();
			cb->param(Event(Event::Done, out));
			return (cb->schedule());
		}
	};

	class MethodOpenSSL : public CryptoHash::Method {
		LogHandle log_;
		FactoryMap<CryptoHash::Algorithm, CryptoHash::Instance> algorithm_map_;
	public:
		MethodOpenSSL(void)
		: CryptoHash::Method("OpenSSL"),
		  log_("/crypto/hash/openssl"),
		  algorithm_map_()
		{
			OpenSSL_add_all_algorithms();

			factory<InstanceEVP> evp_factory;
			algorithm_map_.enter(CryptoHash::MD5, evp_factory(EVP_md5()));
			algorithm_map_.enter(CryptoHash::SHA1, evp_factory(EVP_sha1()));
			algorithm_map_.enter(CryptoHash::SHA256, evp_factory(EVP_sha256()));
			algorithm_map_.enter(CryptoHash::SHA512, evp_factory(EVP_sha512()));
			algorithm_map_.enter(CryptoHash::RIPEMD160, evp_factory(EVP_ripemd160()));

			/* XXX Register.  */
		}

		~MethodOpenSSL()
		{
			/* XXX Unregister.  */
		}

		std::set<CryptoHash::Algorithm> algorithms(void) const
		{
			return (algorithm_map_.keys());
		}

		CryptoHash::Instance *instance(CryptoHash::Algorithm algorithm) const
		{
			return (algorithm_map_.create(algorithm));
		}
	};

	static MethodOpenSSL crypto_hash_method_openssl;
}
