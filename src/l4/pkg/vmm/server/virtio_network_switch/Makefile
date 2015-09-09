PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= virtio-switch
SRC_CC		= main.cc
SRC_CC_IS_CXX11 = y
REQUIRES_LIBS   = libstdc++ libpthread
PRIVATE_INCDIR += $(PKGDIR)/server/include
WARNINGS        = $(WARNINGS_FULL) -Wstrict-aliasing

include $(L4DIR)/mk/prog.mk

CXXFLAGS := $(filter-out -fno-strict-aliasing,$(CXXFLAGS))
