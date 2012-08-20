#ifndef	SSH_SSH_PROTOCOL_H
#define	SSH_SSH_PROTOCOL_H

#include <common/endian.h>

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
		bool decode(Buffer *, Buffer *);
		void encode(Buffer *, Buffer *);
	}

	namespace NameList {
		void encode(Buffer *, const std::vector<Buffer>&);
	}
}

#endif /* !SSH_SSH_PROTOCOL_H */
