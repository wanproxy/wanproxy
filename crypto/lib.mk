VPATH+=	${TOPDIR}/crypto

SRCS+=	crypto_encryption.cc

SRCS+=	crypto_encryption_openssl.cc
SRCS+=	crypto_random_openssl.cc

# Apple has decided to deprecate all of OpenSSL in Mac OS X ~Lion~.  Brilliant.
ifeq "${OSNAME}" "Darwin"
CFLAGS+=	-Wno-deprecated-declarations
endif

LDADD+=		-lcrypto
