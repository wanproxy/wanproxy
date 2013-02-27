/*
 * Copyright (c) 2012 Juli Mallett. All rights reserved.
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

#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <common/buffer.h>

#include <crypto/crypto_hash.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_server_host_key.h>
#include <ssh/ssh_session.h>

namespace {
	class RSAServerHostKey : public SSH::ServerHostKey {
		LogHandle log_;
		SSH::Session *session_; 
		RSA *rsa_;

	public:
		RSAServerHostKey(SSH::Session *session, RSA *rsa)
		: SSH::ServerHostKey("ssh-rsa"),
		  log_("/ssh/serverhostkey/rsa"),
		  session_(session),
		  rsa_(rsa)
		{
			ASSERT(log_, rsa != NULL || session->role_ == SSH::ClientRole);
		}

		~RSAServerHostKey()
		{ }

		SSH::ServerHostKey *clone(void) const
		{
			if (session_->role_ == SSH::ClientRole) {
				if (rsa_ != NULL)
					return (new RSAServerHostKey(session_, RSAPublicKey_dup(rsa_)));
				else
					return (new RSAServerHostKey(session_, NULL));
			} else {
				return (new RSAServerHostKey(session_, RSAPrivateKey_dup(rsa_)));
			}
		}

		bool decode_public_key(Buffer *in)
		{
			ASSERT(log_, session_->role_ == SSH::ClientRole);
			ASSERT(log_, rsa_ == NULL);
			Buffer tag;
			if (!SSH::String::decode(&tag, in))
				return (false);
			if (!tag.equal("ssh-rsa"))
				return (false);
			rsa_ = RSA_new();
			if (rsa_ == NULL)
				return (false);
			if (!SSH::MPInt::decode(&rsa_->e, in))
				return (false);
			if (!SSH::MPInt::decode(&rsa_->n, in))
				return (false);
			return (true);
		}

		void encode_public_key(Buffer *out) const
		{
			SSH::String::encode(out, Buffer("ssh-rsa"));
			SSH::MPInt::encode(out, rsa_->e);
			SSH::MPInt::encode(out, rsa_->n);
		}

		bool sign(Buffer *out, const Buffer *in) const
		{
			ASSERT(log_, session_->role_ == SSH::ServerRole);

			Buffer hash;
			if (!CryptoHash::hash(CryptoHash::SHA1, &hash, in))
				return (false);

			uint8_t m[hash.length()];
			hash.moveout(m, sizeof m);
			uint8_t signature[RSA_size(rsa_)];
			unsigned signature_length = sizeof signature;
			int rv = RSA_sign(NID_sha1, m, sizeof m, signature, &signature_length, rsa_);
			if (rv == 0)
				return (false);
			SSH::String::encode(out, Buffer("ssh-rsa"));
			SSH::String::encode(out, Buffer(signature, signature_length));
			return (true);
		}

		bool verify(const Buffer *signature, const Buffer *message) const
		{
			ASSERT(log_, session_->role_ == SSH::ClientRole);

			Buffer hash;
			if (!CryptoHash::hash(CryptoHash::SHA1, &hash, message))
				return (false);

			Buffer in;
			in.append(signature);
			Buffer tag;
			if (!SSH::String::decode(&tag, &in))
				return (false);
			if (!tag.equal("ssh-rsa"))
				return (false);
			Buffer sig;
			if (!SSH::String::decode(&sig, &in))
				return (false);

			uint8_t m[hash.length()];
			hash.moveout(m, sizeof m);
			uint8_t sigbuf[sig.length()];
			sig.moveout(sigbuf, sig.length());
			int rv = RSA_verify(NID_sha1, m, sizeof m, sigbuf, sizeof sigbuf, rsa_);
			if (rv == 0)
				return (false);
			return (true);
		}

		static RSAServerHostKey *open(SSH::Session *session, FILE *file)
		{
			RSA *rsa;

			ASSERT("/ssh/serverhostkey/rsa/open", session->role_ == SSH::ServerRole);

			rsa = PEM_read_RSAPrivateKey(file, NULL, NULL, NULL);
			if (rsa == NULL)
				return (NULL);

			return (new RSAServerHostKey(session, rsa));
		}
	};
}

void
SSH::ServerHostKey::add_client_algorithms(SSH::Session *session)
{
	ASSERT("/ssh/serverhostkey/client", session->role_ == SSH::ClientRole);
	session->algorithm_negotiation_->add_algorithm(new RSAServerHostKey(session, NULL));
}

SSH::ServerHostKey *
SSH::ServerHostKey::server(SSH::Session *session, const std::string& keyfile)
{
	SSH::ServerHostKey *key;
	FILE *file;

	ASSERT("/ssh/serverhostkey/server", session->role_ == SSH::ServerRole);

	file = fopen(keyfile.c_str(), "r");
	if (file == NULL)
		return (NULL);
	
	key = RSAServerHostKey::open(session, file);
	if (key == NULL) {
		fclose(file);
		return (NULL);
	}

	fclose(file);

	return (key);
}
