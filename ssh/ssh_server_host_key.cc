#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <common/buffer.h>

#include <crypto/crypto_hash.h>

#include <ssh/ssh_protocol.h>
#include <ssh/ssh_server_host_key.h>

namespace {
	class RSAServerHostKey : public SSH::ServerHostKey {
		LogHandle log_;
		RSA *rsa_;

		RSAServerHostKey(RSA *rsa)
		: SSH::ServerHostKey("ssh-rsa"),
		  log_("/ssh/serverhostkey/rsa"),
		  rsa_(rsa)
		{ }
	public:

		~RSAServerHostKey()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}

		void encode_public_key(Buffer *out) const
		{
			Buffer tag("ssh-rsa");

			SSH::String::encode(out, &tag);
			SSH::MPInt::encode(out, rsa_->e);
			SSH::MPInt::encode(out, rsa_->n);
		}

		bool sign(Buffer *out, const Buffer *in) const
		{
			const CryptoHash::Method *method = CryptoHash::Method::method(CryptoHash::SHA1);
			CryptoHash::Instance *instance = method->instance(CryptoHash::SHA1);
			Buffer hash;
			if (!instance->hash(&hash, in))
				return (false);

			uint8_t m[hash.length()];
			hash.moveout(m, sizeof m);
			uint8_t signature[RSA_size(rsa_)];
			unsigned signature_length;
			int rv = RSA_sign(NID_sha1, m, sizeof m, signature, &signature_length, rsa_);
			if (rv == 0)
				return (false);
			out->append(signature, signature_length);
			return (true);
		}

		static RSAServerHostKey *open(FILE *file)
		{
			RSA *rsa;

			rsa = PEM_read_RSAPrivateKey(file, NULL, NULL, NULL);
			if (rsa == NULL)
				return (NULL);

			return (new RSAServerHostKey(rsa));
		}
	};
}

SSH::ServerHostKey *
SSH::ServerHostKey::server(const std::string& keyfile)
{
	SSH::ServerHostKey *key;
	FILE *file;

	file = fopen(keyfile.c_str(), "r");
	if (file == NULL)
		return (NULL);
	
	key = RSAServerHostKey::open(file);
	if (key == NULL) {
		fclose(file);
		return (NULL);
	}

	fclose(file);

	return (key);
}
