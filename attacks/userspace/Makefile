
.PHONY: all clean

all:	remote-ps snarf-sshkey

remote-ps: remote-ps.c
	gcc remote-ps.c -Wall -W -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o remote-ps

snarf-sshkey: snarf-sshkey.c ssh-authfile.h
	gcc snarf-sshkey.c -Wall -W -lcrypto -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o snarf-sshkey

clean:
	-rm remote-ps snarf-sshkey
