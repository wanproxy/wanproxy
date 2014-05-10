/*
 * Copyright (c) 2012-2013 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>

#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_session.h>

void
SSH::Session::activate_chosen(void)
{
	const Buffer *local_to_remote_iv_;
	const Buffer *remote_to_local_iv_;
	const Buffer *local_to_remote_key_;
	const Buffer *remote_to_local_key_;

	/*
	 * XXX
	 * Need to free instances in active_algorithms_.
	 */

	active_algorithms_ = chosen_algorithms_;

	if (active_algorithms_.client_to_server_.encryption_ != NULL) {
		client_to_server_iv_ = generate_key("A", active_algorithms_.client_to_server_.encryption_->iv_size());
		client_to_server_key_ = generate_key("C", active_algorithms_.client_to_server_.encryption_->key_size());
	}
	if (active_algorithms_.server_to_client_.encryption_ != NULL) {
		server_to_client_iv_ = generate_key("B", active_algorithms_.server_to_client_.encryption_->iv_size());
		server_to_client_key_ = generate_key("D", active_algorithms_.server_to_client_.encryption_->key_size());
	}
	if (active_algorithms_.client_to_server_.mac_ != NULL) {
		client_to_server_integrity_key_ = generate_key("E", active_algorithms_.client_to_server_.mac_->key_size());
		if (!active_algorithms_.client_to_server_.mac_->initialize(&client_to_server_integrity_key_))
			HALT("/ssh/session") << "Failed to activate client-to-server MAC.";
	}
	if (active_algorithms_.server_to_client_.mac_ != NULL) {
		server_to_client_integrity_key_ = generate_key("F", active_algorithms_.server_to_client_.mac_->key_size());
		if (!active_algorithms_.server_to_client_.mac_->initialize(&server_to_client_integrity_key_))
			HALT("/ssh/session") << "Failed to activate server-to-client MAC.";
	}

	if (active_algorithms_.local_to_remote_->encryption_ != NULL) {
		if (role_ == ClientRole) {
			local_to_remote_iv_ = &client_to_server_iv_;
			local_to_remote_key_ = &client_to_server_key_;
		} else {
			local_to_remote_iv_ = &server_to_client_iv_;
			local_to_remote_key_ = &server_to_client_key_;
		}
		if (!active_algorithms_.local_to_remote_->encryption_->initialize(CryptoEncryption::Encrypt, local_to_remote_key_, local_to_remote_iv_))
			HALT("/ssh/session") << "Failed to initialize local-to-remote encryption.";
	}

	if (active_algorithms_.remote_to_local_->encryption_ != NULL) {
		if (role_ == ClientRole) {
			remote_to_local_iv_ = &server_to_client_iv_;
			remote_to_local_key_ = &server_to_client_key_;
		} else {
			remote_to_local_iv_ = &client_to_server_iv_;
			remote_to_local_key_ = &client_to_server_key_;
		}
		if (!active_algorithms_.remote_to_local_->encryption_->initialize(CryptoEncryption::Decrypt, remote_to_local_key_, remote_to_local_iv_))
			HALT("/ssh/session") << "Failed to initialize local-to-remote encryption.";
	}
}

Buffer
SSH::Session::generate_key(const std::string& x, unsigned key_size)
{
	Buffer key;
	while (key.length() < key_size) {
		Buffer input;
		input.append(shared_secret_);
		input.append(exchange_hash_);
		if (key.empty()) {
			input.append(x);
			input.append(session_id_);
		} else {
			input.append(key);
		}
		if (!active_algorithms_.key_exchange_->hash(&key, &input))
			HALT("/ssh/session") << "Hash failed in generating key.";
	}
	if (key.length() > key_size)
		key.truncate(key_size);
	return (key);
}
