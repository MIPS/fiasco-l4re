PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SKLIBDIR ?= /tmp/sklib/
include ${SKLIBDIR}/incpaths
include ${SKLIBDIR}/swldlibs

CXXFLAGS += ${ELPSK_INCPATH}
LDFLAGS  += ${ELPSK_SWLDLIBS}

TARGET		= sk_hello
SRC_CC		= ../../../sk_ipc/server/src/trustlet_srv1.cc main.cc


include $(L4DIR)/mk/prog.mk
