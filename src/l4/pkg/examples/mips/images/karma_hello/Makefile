PKGDIR    ?= ../../..
KARMADIR = $(abspath $(PKGDIR)/../karma)
L4DIR    ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

CC        = $(SYSTEM_TARGET)gcc -mips32r2 -EL

CPPFLAGS  = -I./asm -I./ -I$(PKGDIR)/mips/include -I$(KARMADIR)/include -g -O2
ASFLAGS   = -xassembler-with-cpp
LDFLAGS   = -static -nostdlib -Wl,-Ttext-segment=0x80010000
LDLIBS    = -lc `$(CC) -print-libgcc-file-name`

OBJ       = crt0.o karma_hello.o hypercall.o

# using var $(TARGET) interferes with L4 build
ifneq ($(wildcard $(KARMADIR)), )
TARG      = karma_hello
else
TARG      =
  ifneq ($(MAKECMDGOALS), clean)
  $(warning ====== NOTICE ======)
  $(warning ======        $(KARMADIR) is not present)
  $(warning ======        Skipping $(TARG))
  $(warning ====== NOTICE ======)
  endif
endif

all:: $(TARG)

$(TARG): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LOADLIBES) $(LDLIBS) -o $@

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

%.o : %.S
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

clean:
	- rm -f $(TARG) *.o
