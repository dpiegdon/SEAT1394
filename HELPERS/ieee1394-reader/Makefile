
.PHONY: clean all


CFLAGS= -fomit-frame-pointer -O3 -g
LDFLAGS= -lraw1394 -fpic -g

all:	reader

reader:	reader.o
	gcc ${LDFLAGS} reader.o -o reader

clean:
	-rm *.o
	-rm reader memrd

