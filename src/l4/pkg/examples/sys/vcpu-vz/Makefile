PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= ex_vcpuvz
SYSTEMS                = mips-l4f
SRC_CC		= vcpu-vz.cc
REQUIRES_LIBS   = libvcpu cxx_io cxx_libc_io
DEPENDS_PKGS    = libvcpu

include $(L4DIR)/mk/prog.mk
