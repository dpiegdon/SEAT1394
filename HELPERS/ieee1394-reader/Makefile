
.PHONY: clean all


CFLAGS= -fomit-frame-pointer -O3 -g
LDFLAGS= -lraw1394 -fpic -g

all:	reader memrd

reader:	reader.o
	gcc ${LDFLAGS} reader.o -o reader

memrd:	memrd.o
	gcc memrd.o -o memrd

clean:
	-rm *.o
	-rm reader memrd

