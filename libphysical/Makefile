
.PHONY : all clean install uninstall

CC = gcc

PREFIX ?= $(HOME)
RMDIR ?= $(shell which rmdir)

OBJECTS = $(patsubst %.c,%.o,$(shell echo *.c))
HEADERS = $(shell echo *.h)

CFLAGS += -O9 -fPIC -finline-functions  -ffast-math  -funroll-all-loops -Wall -Wpointer-arith -D_REENTRANT
LDFLAGS+= -shared -lraw1394

all:	libphysical.so
	-

clean:
	-$(RM) $(OBJECTS)
	-$(RM) libphysical.so

install: all
	strip libphysical.so
	mkdir -p ${PREFIX}/lib/
	mkdir -p ${PREFIX}/include/
	install -m 0555 libphysical.so ${PREFIX}/lib/
	install -m 0444 physical.h ${PREFIX}/include/

uninstall:
	$(RM) ${PREFIX}/lib/libphysical.so
	$(RM) ${PREFIX}/include/physical.h
	-$(RMDIR) $(HOME)/lib/
	-$(RMDIR) $(HOME)/include/

$(OBJECTS): $(HEADERS) Makefile

libphysical.so: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
