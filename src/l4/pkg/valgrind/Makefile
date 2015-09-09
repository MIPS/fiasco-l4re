PKGDIR	?= .
L4DIR	?= $(PKGDIR)/../..

# the default is to build the listed directories, provided that they
# contain a Makefile. If you need to change this, uncomment the following
# line and adapt it.
# TARGET = include src lib server examples doc

SYSTEMS = x86-l4f

all::
	if [ ! -e $(PKGDIR)/broken ]; then PWD=$(PWD)/build make -C build; fi

clean::
	PWD=$(PWD)/build make -C build $@

cleanall::
	PWD=$(PWD)/build make -C build $@

include $(L4DIR)/mk/subdir.mk
