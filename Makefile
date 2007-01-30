#   $Id$
#   install headers and libraries
# 
#   Copyright (C) 2006,2007
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

PREFIX ?= $(HOME)

.PHONY : all clean install uninstall headers-install libphysical-installliblinear-install

all:		install
	
install:	headers-install libphysical-install liblinear-install

uninstall:	headers-uninstall libphysical-uninstall liblinear-uninstall

clean:		libphysical-clean liblinear-clean

headers-install:
	mkdir -p $(PREFIX)/include/ia32
	cp include/ia32/* $(PREFIX)/include/ia32/
	cp include/endian_swap.h $(PREFIX)/include/

headers-uninstall:
	-rm -R $(PREFIX)/include/ia32
	-rm $(PREFIX)/include/endian_swap.h


libphysical-install:
	make -C libphysical install

libphysical-uninstall:
	make -C libphysical uninstall

libphysical-clean:
	make -C libphysical clean


liblinear-install:
	make -C liblinear install

liblinear-uninstall:
	make -C liblinear uninstall

liblinear-clean:
	make -C liblinear clean
