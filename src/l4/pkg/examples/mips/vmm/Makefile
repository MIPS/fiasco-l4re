PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = ex_vmm
SYSTEMS         = mips-l4f
SRC_CC          = main.cc vmm.cc mem_man.cc loader.cc \
                  serial.cc l4_vcpu.cc l4_ser.cc devices/devices.cc
REQUIRES_LIBS   = libvcpu l4re_c-util l4re-util libpthread libstdc++
PRIVATE_INCDIR  = $(PKGDIR)/mips/include

include $(L4DIR)/mk/prog.mk
