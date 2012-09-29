#ifndef	SSH_SSH_SESSION_H
#define	SSH_SSH_SESSION_H

namespace SSH {
	class AlgorithmNegotiation;

	enum Role {
		ClientRole,
		ServerRole,
	};

	struct Session {
		Role role_;
		AlgorithmNegotiation *algorithm_negotiation_;
		Buffer client_version_;	/* Client's version string.  */
		Buffer server_version_;	/* Server's version string.  */
		Buffer client_kexinit_;	/* Client's first key exchange packet.  */
		Buffer server_kexinit_;	/* Server's first key exchange packet.  */
		Buffer shared_secret_;	/* Shared secret from key exchange.  */
		Buffer session_id_;	/* First exchange hash.  */
		Buffer exchange_hash_;	/* Most recent exchange hash.  */

		Session(Role role)
		: role_(role),
		  algorithm_negotiation_(NULL),
		  client_version_(),
		  server_version_(),
		  client_kexinit_(),
		  server_kexinit_(),
		  shared_secret_(),
		  session_id_(),
		  exchange_hash_()
		{ }

		void local_version(const Buffer& version)
		{
			if (role_ == ClientRole)
				client_version_ = version;
			else
				server_version_ = version;
		}

		void remote_version(const Buffer& version)
		{
			if (role_ == ClientRole)
				server_version_ = version;
			else
				client_version_ = version;
		}

		void local_kexinit(const Buffer& kexinit)
		{
			if (role_ == ClientRole)
				client_kexinit_ = kexinit;
			else
				server_kexinit_ = kexinit;
		}

		void remote_kexinit(const Buffer& kexinit)
		{
			if (role_ == ClientRole)
				server_kexinit_ = kexinit;
			else
				client_kexinit_ = kexinit;
		}
	};
}

#endif /* !SSH_SSH_SESSION_H */
