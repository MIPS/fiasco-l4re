PKGDIR       ?= ../..
L4DIR        ?= $(PKGDIR)/../..

CPPFLAGS += -DDEBUG=1

SKLIBDIR ?= /tmp/sklib/
include ${SKLIBDIR}/incpaths
include ${SKLIBDIR}/swldlibs

CXXFLAGS += ${ELPSK_INCPATH}
LDFLAGS  += ${ELPSK_SWLDLIBS}

#PC_FILENAME  =  libtee_clnt
TARGET       =  tee_clnt trustlet_srv1 trustlet_srv2 trustlet_srv3
SRC_CC_trustlet_srv1 = trustlet_srv1.cc SKTAPP_receive_operation_iface_srv1.cc
SRC_CC_trustlet_srv2 = trustlet_srv2.cc SKTAPP_receive_operation_iface_srv1.cc
SRC_CC_trustlet_srv3 = trustlet_srv3.cc SKTAPP_receive_operation_iface_srv1.cc
SRC_CC_tee_clnt = tee_clnt.cc

REQUIRES_LIBS = libtee_clnt

include $(L4DIR)/mk/prog.mk
