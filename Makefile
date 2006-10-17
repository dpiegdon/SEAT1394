
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
