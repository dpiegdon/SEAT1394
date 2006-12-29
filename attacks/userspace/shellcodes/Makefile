#   $Id$
#   shellcodes to be injected
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
CFLAGS=-g
AS=nasm

all:	testcode print.o marker.o

clean:
	-rm -f marker.o print.o *.lst testcode

print.o: print.s
	nasm -l print.lst -o print.o print.s

marker.o: marker.s
	nasm -l marker.lst -o marker.o marker.s
