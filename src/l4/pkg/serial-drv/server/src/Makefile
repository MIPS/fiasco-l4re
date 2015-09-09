PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = serial-drv
SRC_CC          = main.cc
REQUIRES_LIBS   = libc_support_misc libio drivers_uart

DEFINES_mips-l4f += -DUSE_MALTA_UART

include $(L4DIR)/mk/prog.mk
