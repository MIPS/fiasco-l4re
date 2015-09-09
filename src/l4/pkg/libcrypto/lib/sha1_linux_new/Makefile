PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_sha1_linux_new.a $(PKGNAME)_sha1_linux_new.p.a
SYSTEMS = x86 arm amd64
PC_FILENAME = $(PKGNAME)_sha1_linux_new

# list your .c files here
SRC_C	= sha1.c sha1_generic.c

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
