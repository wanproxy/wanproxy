VPATH+=	${TOPDIR}/crypto

SRCS+=	crypto_encryption_openssl.cc

LDADD+=		-lcrypto
