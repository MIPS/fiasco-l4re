PKGDIR       ?= ../..
L4DIR        ?= $(PKGDIR)/../..

PC_FILENAME  =  libtee_clnt
TARGET       =  libtee_clnt.a libtee_clnt.so
SYSTEMS      =  mips-l4f

CPPFLAGS += -DDEBUG=1
SKLIBDIR ?= /tmp/sklib/
include ${SKLIBDIR}/incpaths
include ${SKLIBDIR}/swldlibs

CXXFLAGS += ${ELPSK_INCPATH}
LDFLAGS  += ${ELPSK_SWLDLIBS}

#SRC_CC_trustlet_srv1 = trustlet_srv1.cc SKTAPP_receive_operation_iface_srv1.cc
SRC_CC = tee_trustlet_iface.cc

include $(L4DIR)/mk/lib.mk
