VPATH+=	${TOPDIR}/network

ifndef USE_PCAP
ifeq "${OSNAME}" "Darwin"
USE_PCAP=	yes
endif

ifeq "${OSNAME}" "FreeBSD"
USE_PCAP=	yes
endif

ifndef USE_PCAP
USE_PCAP=	no
endif
endif

ifeq "${USE_PCAP}" "yes"
LDADD+=	-lpcap
USE_NETWORK_INTERFACES=	pcap
endif

ifdef USE_NETWORK_INTERFACES
SRCS+=	network_interface.cc
SRCS+=	$(addprefix network_interface_,$(addsuffix .cc, ${USE_NETWORK_INTERFACES}))
endif

NET_REQUIRES=	event io
