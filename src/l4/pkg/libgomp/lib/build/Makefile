PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifneq ($(SYSTEM),)
  # check whether used gcc has the omp.h file available
  OMP_H_PATH := $(shell $(CC) -print-file-name=include/omp.h)

  ifeq ($(strip $(OMP_H_PATH)),include/omp.h)
    $(info [32m $(CC) does not have omp.h header file available, skipping. [0m)
    PC_FILENAME =
  else
   TARGET           = libgomp.a
 endif
endif

ifneq ($(filter 4.2 4.3 4.4 4.5 4.6,$(GCCVERSION)),)
  USE_VERSION = 4.3
else
  ifneq ($(filter 4.7 4.8,$(GCCVERSION)),)
    USE_VERSION = 4.7
  else
    USE_VERSION = 4.9
  endif
endif

PRIVATE_INCDIR = $(PKGDIR)/lib/contrib/gcc-$(USE_VERSION)/libgomp \
                 $(PKGDIR)/lib/contrib/gcc-$(USE_VERSION)/libgomp/config/posix \
                 $(PKGDIR)/lib/build \
                 $(PKGDIR)/lib/build/ARCH-$(BUILD_ARCH)/gcc-$(USE_VERSION)

vpath %.c $(PKGDIR)/lib/contrib/gcc-$(USE_VERSION)/libgomp \
          $(PKGDIR)/lib/contrib/gcc-$(USE_VERSION)/libgomp/config/posix

SRC_C     = affinity.c alloc.c critical.c error.c iter.c loop.c ordered.c \
	    sections.c work.c barrier.c env.c fortran.c parallel.c proc.c sem.c \
            time.c team.c lock.c mutex.c single.c bar.c
SRC_C-4.7 = task.c iter_ull.c loop_ull.c ptrlock.c 
SRC_C-4.9 = task.c iter_ull.c loop_ull.c ptrlock.c target.c
SRC_C    += $(SRC_C-$(USE_VERSION))

REQUIRES_LIBS = libpthread
WARNINGS      = $(WARNINGS_MINIMAL)

include $(L4DIR)/mk/lib.mk
