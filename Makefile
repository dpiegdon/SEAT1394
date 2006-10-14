
PREFIX ?= $(HOME)

.PHONY : all clean install uninstall headers-install libphysical-installliblogical-install

all:		install
	
install:	headers-install libphysical-install liblogical-install

uninstall:	headers-uninstall libphysical-uninstall liblogical-uninstall

clean:		libphysical-clean liblogical-clean

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


liblogical-install:
	make -C liblogical install

liblogical-uninstall:
	make -C liblogical uninstall

liblogical-clean:
	make -C liblogical clean
