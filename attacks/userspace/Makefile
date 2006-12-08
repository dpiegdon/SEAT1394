
.PHONY: all clean

CC=gcc

PREFIX?=$(HOME)

CFLAGS += -I$(PREFIX)/include -O9 -W -Wall -fPIC -finline-functions -ffast-math -funroll-all-loops -Wpointer-arith -DUSETERM
LDFLAGS+= -L$(PREFIX)/lib

all:	debug-sshkey remote-ps snarf-sshkey

clean:
	-rm -f remote-ps snarf-sshkey debug-sshkey *.o



remote-ps: LDFLAGS += -lphysical -llinear
remote-ps: remote-ps.o proc_info.o

snarf-sshkey: LDFLAGS += -lphysical -llinear -lcrypto
snarf-sshkey: snarf-sshkey.o sshkey-sanitychecks.o proc_info.o

snarf-sshkey-static: LDFLAGS += -lphysical -llinear -static -lcrypto -lraw1394 -ldl -lbz2
snarf-sshkey-static: snarf-sshkey.o sshkey-sanitychecks.o proc_info.o
	$(CC) snarf-sshkey.o sshkey-sanitychecks.o proc_info.o $(LDFLAGS) -o $@

debug-sshkey: LDFLAGS += -lm -lcrypto
debug-sshkey: debug-sshkey.o sshkey-sanitychecks.o



remote-ps.o: remote-ps.c

proc_info.o: proc_info.c

sshkey-sanitychecks.o: sshkey-sanitychecks.h sshkey-sanitychecks.c term.h

snarf-sshkey.o: snarf-sshkey.c ssh-authfile.h term.h

debug-sshkey.o: debug-sshkey.c ssh-authfile.h term.h

