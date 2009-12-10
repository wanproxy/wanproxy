VPATH+=${TOPDIR}/config

SRCS+=	config.cc
SRCS+=	config_class.cc
SRCS+=	config_class_log_mask.cc
SRCS+=	config_object.cc
SRCS+=	config_type_int.cc
SRCS+=	config_type_log_level.cc
SRCS+=	config_type_pointer.cc
SRCS+=	config_type_string.cc

define __per_library
_lib:=$(1)
ifeq "${_lib}" "io"
SRCS+=	config_class_address.cc
SRCS+=	config_type_address_family.cc
endif
endef

$(foreach _lib, ${USE_LIBS}, $(eval $(call __per_library, ${_lib})))
