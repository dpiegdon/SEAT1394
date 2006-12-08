#   $Id$
#   libphysical
# 
#   Copyright (C) 2006
#   losTrace aka "David R. Piegdon"
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2, as
#   published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

.PHONY : all clean install uninstall

CC = gcc
AR = ar

PREFIX ?= $(HOME)
RMDIR ?= $(shell which rmdir)

OBJECTS = $(patsubst %.c,%.o,$(shell echo *.c))
HEADERS = $(shell echo *.h)

CFLAGS += -O9 -fvisibility=hidden -fPIC -finline-functions  -ffast-math  -funroll-all-loops -Wall -Wpointer-arith -D_REENTRANT
LDFLAGS+= -shared -lraw1394 -fvisibility=hidden

all:	libphysical.so libphysical.a
	-

clean:
	-$(RM) $(OBJECTS)
	-$(RM) libphysical.so
	-$(RM) libphysical.a

install: all
	strip libphysical.so
	mkdir -p ${PREFIX}/lib/
	mkdir -p ${PREFIX}/include/
	install -m 0555 libphysical.so ${PREFIX}/lib/
	install -m 0444 libphysical.a ${PREFIX}/lib/
	install -m 0444 physical.h ${PREFIX}/include/

uninstall:
	$(RM) ${PREFIX}/lib/libphysical.so
	$(RM) ${PREFIX}/lib/libphysical.a
	$(RM) ${PREFIX}/include/physical.h
	-$(RMDIR) $(HOME)/lib/
	-$(RMDIR) $(HOME)/include/

$(OBJECTS): $(HEADERS) Makefile

libphysical.so: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
libphysical.a: $(OBJECTS)
	$(AR) -rvc $@ $(OBJECTS)
