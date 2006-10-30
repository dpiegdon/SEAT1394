
.PHONY: all clean

all:	remote-ps snarf-sshkey

remote-ps: remote-ps.c
	gcc remote-ps.c -Wall -W -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o remote-ps

snarf-sshkey: snarf-sshkey.o
	gcc snarf-sshkey.o openssh/entropy.o -lm -lresolv -lcrypto -lcrypt -lutil -lz -lnsl   -lssh -lopenbsd-compat   -lphysical -llinear \
		-L ./openssh/ -L ./openssh/openbsd-compat/ -L${HOME}/lib -o snarf-sshkey

snarf-sshkey.o: snarf-sshkey.c ssh-authfile.h
	gcc snarf-sshkey.c -Wall -W -I/home/lostrace/include -c -o snarf-sshkey.o

clean:
	-rm -f remote-ps snarf-sshkey *.o
