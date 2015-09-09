PKGDIR = .
L4DIR  ?= $(PKGDIR)/../..

TARGET = lib server

CPPFLAGS += -DDEBUG=1

ifndef BUILD_SK_IPC
TARGET = include        
else
TARGET = $(wildcard [a-z]*)    
endif

include $(L4DIR)/mk/subdir.mk

