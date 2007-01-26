#   $Id$
#   userspace attacks
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

.PHONY: all clean

CC=gcc

PREFIX?=$(HOME)

CFLAGS += -I$(PREFIX)/include -O9 -W -Wall -fPIC -finline-functions -ffast-math -funroll-all-loops -Wpointer-arith -DUSETERM
LDFLAGS+= -L$(PREFIX)/lib -s

all:	inject-code show-stack victim rawshell

clean:
	-rm -f *.o inject-code show-stack victim



inject-code: LDFLAGS += -lphysical -llinear
inject-code: inject-code.o proc_info.o

rawshell: LDFLAGS += -lphysical -llinear
rawshell: rawshell.o proc_info.o

show-stack: LDFLAGS += -lphysical -llinear
show-stack: show-stack.o proc_info.o

