VPATH+=	${TOPDIR}/config

SRCS+=	config.cc
SRCS+=	config_class.cc
SRCS+=	config_class_log_mask.cc
SRCS+=	config_object.cc
SRCS+=	config_type_int.cc
SRCS+=	config_type_log_level.cc
SRCS+=	config_type_pointer.cc
SRCS+=	config_type_string.cc
SRCS+=	config_value.cc

SRCS_io_socket+=config_class_address.cc
SRCS_io_socket+=config_type_address_family.cc
