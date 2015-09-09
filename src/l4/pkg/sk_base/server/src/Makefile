PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SKLIBDIR ?= /tmp/sklib/
include ${SKLIBDIR}/incpaths
include ${SKLIBDIR}/swldlibs

CXXFLAGS += ${ELPSK_INCPATH}
LDFLAGS  += ${ELPSK_SWLDLIBS}

TARGET		= sk_base
SRC_CC		= main.cc 
REQUIRES_LIBS = libtee_clnt


include $(L4DIR)/mk/prog.mk
