PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

SYSTEMS         = arm-l4f
TARGET          = vmm
SRC_CC          = gic.cc main.cc
REQUIRES_LIBS   = libstdc++ libio-vbus libfdt
DEFAULT_RELOC   = 0xa0000000
SRC_CC_IS_CXX11 = y
CXXFLAGS       += -O3
PRIVATE_INCDIR += $(PKGDIR)/server/include

include $(L4DIR)/mk/prog.mk

CXXFLAGS := $(filter-out -march%,$(CXXFLAGS)) -mcpu=cortex-a15
CXXFLAGS := $(filter-out -fno-strict-aliasing,$(CXXFLAGS))

#guest_simple.o: /home/adam/dev/tudos-l4re/l4/pkg/portus/guests/simple/simpleguest.raw /tmp/out-virt/bootstrap.raw
