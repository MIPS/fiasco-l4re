PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

TARGET        = ex_shmc ex_shmc_prod ex_shmc_cons
SRC_C_ex_shmc         = prodcons.c
SRC_C_ex_shmc_prod    = prod.c
SRC_C_ex_shmc_cons    = cons.c
DEPENDS_PKGS  = shmc
REQUIRES_LIBS = shmc libpthread

include $(L4DIR)/mk/prog.mk
