/*
 * Copyright (c) 2011-2012 Juli Mallett. All rights reserved.
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

#ifndef	SSH_SSH_PROTOCOL_H
#define	SSH_SSH_PROTOCOL_H

#include <common/endian.h>

/* XXX OpenSSL dependency.  */
typedef struct bignum_st BIGNUM;

namespace SSH {
	namespace Message {
		static const uint8_t
			TransportRangeBegin = 1,
			TransportDisconnectMessage = 1,
			TransportIgnoreMessage = 2,
			TransportUnimplementedMessage = 3,
			TransportDebugMessage = 4,
			TransportServiceRequestMessage = 5,
			TransportServiceAcceptMessage = 6,
			TransportRangeEnd = 19,

			AlgorithmNegotiationRangeBegin = 20,
			KeyExchangeInitializationMessage = 20,
			NewKeysMessage = 21,
			AlgorithmNegotiationRangeEnd = 29,

			KeyExchangeMethodRangeBegin = 30,
			KeyExchangeMethodRangeEnd = 49,

			UserAuthenticationGenericRangeBegin = 50,
			UserAuthenticationRequestMessage = 50,
			UserAuthenticationFailureMessage = 51,
			UserAuthenticationSuccessMessage = 52,
			UserAuthenticationBannerMessage = 53,
			UserAuthenticationGenericRangeEnd = 59,

			UserAuthenticationMethodRangeBegin = 60,
			UserAuthenticationMethodRangeEnd = 79,

			ConnectionProtocolGlobalRangeBegin = 80,
			ConnectionProtocolGlobalRequestMessage = 80,
			ConnectionProtocolGlobalRequestSuccessMessage = 81,
			ConnectionProtocolGlobalRequestFailureMessage = 82,
			ConnectionProtocolGlobalRangeEnd = 89,

			ConnectionChannelRangeBegin = 90,
			ConnectionChannelOpen = 90,
			ConnectionChannelOpenConfirmation = 91,
			ConnectionChannelOpenFailure = 92,
			ConnectionChannelWindowAdjust = 93,
			ConnectionChannelData = 94,
			ConnectionChannelExtendedData = 95,
			ConnectionChannelEndOfFile = 96,
			ConnectionChannelClose = 97,
			ConnectionChannelRequest = 98,
			ConnectionChannelRequestSuccess = 99,
			ConnectionChannelRequestFailure = 100,
			ConnectionChannelRangeEnd = 127,

			ClientProtocolReservedRangeBegin = 128,
			ClientProtocolReservedRangeEnd = 191,

			LocalExtensionRangeBegin = 192,
			LocalExtensionRangeEnd = 255;
	}

	namespace Boolean {
		static const uint8_t
			True = 1,
			False = 0;
	}

	namespace Disconnect {
		static const uint8_t
			HostNotallowedToConnect = 1,
			ProtocolError = 2,
			KeyExchangeFailed = 3,
			Reserved = 4,
			MACError = 5,
			CompressionError = 6,
			ServiceNotAvailable = 7,
			ProtocolVersionNotSupported = 8,
			HostKeyNotVerifiable = 9,
			ConnectionLost = 10,
			ByApplication = 11,
			TooManyConnections = 12,
			AuthenticationCancelledByUser = 13,
			NoMoreAuthenticationMethodsAvailable = 14,
			IllegalUserName = 15;
	}

	namespace String {
		void encode(Buffer *, const Buffer&);
		void encode(Buffer *, Buffer *);
		bool decode(Buffer *, Buffer *);
	}

	namespace UInt32 {
		void encode(Buffer *, uint32_t);
		bool decode(uint32_t *, Buffer *);
	}

	namespace MPInt {
		void encode(Buffer *, const BIGNUM *);
		bool decode(BIGNUM **, Buffer *);
	}

	namespace NameList {
		void encode(Buffer *, const std::vector<Buffer>&);
		bool decode(std::vector<Buffer>&, Buffer *);
	}
}

#endif /* !SSH_SSH_PROTOCOL_H */
