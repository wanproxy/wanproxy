.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/config

SRCS+=	config.cc
SRCS+=	config_class.cc
SRCS+=	config_class_log_mask.cc
SRCS+=	config_object.cc
SRCS+=	config_type_int.cc
SRCS+=	config_type_string.cc
