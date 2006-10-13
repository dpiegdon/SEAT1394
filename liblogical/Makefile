
.PHONY : all clean install uninstall

CC = gcc

PREFIX ?= $(HOME)
RMDIR ?= $(shell which rmdir)

OBJECTS = $(patsubst %.c,%.o,$(shell echo *.c))
HEADERS = $(shell echo *.h)

CFLAGS += -O9 -fPIC -finline-functions  -ffast-math  -funroll-all-loops -Wall -Wpointer-arith -D_REENTRANT -I${PREFIX}/include
LDFLAGS+= -shared -lphysical -L$(PREFIX)/lib

all:	liblogical.so
	-

clean:
	-$(RM) $(OBJECTS)
	-$(RM) liblogical.so

install: all
	strip liblogical.so
	mkdir -p ${PREFIX}/lib/
	mkdir -p ${PREFIX}/include/
	install -m 0555 liblogical.so ${PREFIX}/lib/
	install -m 0444 logical.h ${PREFIX}/include/

uninstall:
	$(RM) ${PREFIX}/lib/liblogical.so
	$(RM) ${PREFIX}/include/logical.h
	-$(RMDIR) $(HOME)/lib/
	-$(RMDIR) $(HOME)/include/

$(OBJECTS): $(HEADERS) Makefile

liblogical.so: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
