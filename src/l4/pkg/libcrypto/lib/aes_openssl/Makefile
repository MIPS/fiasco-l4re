PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_aes_openssl.a $(PKGNAME)_aes_openssl.p.a
SYSTEMS = x86
PC_FILENAME = $(PKGNAME)_aes_openssl

# list your .c files here
SRC_C	= glue.c
SRC_S	= ax86-elf.S

PRIVATE_INCDIR = $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
