.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/config

SRCS+=	config.cc
SRCS+=	config_class.cc
SRCS+=	config_object.cc
