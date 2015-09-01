PKGDIR   ?= ../../..
L4DIR    ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

CC        = $(SYSTEM_TARGET)gcc $(CARCHFLAGS)

CPPFLAGS  = -I./ -I../../include -g
ASFLAGS   = -xassembler-with-cpp
LDFLAGS   = -static -nostdlib -Wl,-Ttext-segment=0x80010000
LDLIBS    = -lc `$(CC) -print-libgcc-file-name`

OBJ       = crt0.o guest_hello.o hypercall.o
# using var $(TARGET) interferes with L4 build
TARG      = guest_hello

all:: $(TARG)

$(TARG): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LOADLIBES) $(LDLIBS) -o $@

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

%.o : %.S
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

distclean: clean
	- rm -f *~

clean:
	- rm -f $(TARG) *.o

