PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          ?= libfdt.a libfdt.so
SRC_C           = $(LIBFDT_SRCS)
PRIVATE_INCDIR  = $(SRC_DIR)/../contrib
CONTRIB_INCDIR  = libfdt
WARNINGS        = $(WARNINGS_MEDIUM)

vpath %.c $(SRC_DIR)/../contrib

include $(L4DIR)/mk/lib.mk
include $(SRC_DIR)/../contrib/Makefile.libfdt
