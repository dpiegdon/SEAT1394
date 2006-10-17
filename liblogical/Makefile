
.PHONY : all clean install uninstall

CC = gcc

PREFIX ?= $(HOME)
RMDIR ?= $(shell which rmdir)

OBJECTS = $(patsubst %.c,%.o,$(shell echo *.c))
HEADERS = $(shell echo *.h)

CFLAGS += -O9 -fPIC -finline-functions  -ffast-math  -funroll-all-loops -Wall -Wpointer-arith -D_REENTRANT -I${PREFIX}/include
LDFLAGS+= -shared -lphysical -L$(PREFIX)/lib -lbz2

all:	liblinear.so
	-

clean:
	-$(RM) $(OBJECTS)
	-$(RM) liblinear.so

install: all
	strip liblinear.so
	mkdir -p ${PREFIX}/lib/
	mkdir -p ${PREFIX}/include/
	install -m 0555 liblinear.so ${PREFIX}/lib/
	install -m 0444 linear.h ${PREFIX}/include/

uninstall:
	$(RM) ${PREFIX}/lib/liblinear.so
	$(RM) ${PREFIX}/include/linear.h
	-$(RMDIR) $(HOME)/lib/
	-$(RMDIR) $(HOME)/include/

$(OBJECTS): $(HEADERS) Makefile

liblinear.so: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
