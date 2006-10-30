
.PHONY: all clean

all:	remote-ps snarf-sshkey

remote-ps: remote-ps.c
	gcc remote-ps.c -Wall -W -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o remote-ps

snarf-sshkey: ssh-authfile.o snarf-sshkey.o
	gcc ssh-authfile.o snarf-sshkey.o -lcrypto -lphysical -llinear -L/home/lostrace/lib -o snarf-sshkey

ssh-authfile.o: ssh-authfile.c ssh-authfile.h
	gcc ssh-authfile.c -Wall -W -c -o ssh-authfile.o
snarf-sshkey.o: snarf-sshkey.c ssh-authfile.h
	gcc snarf-sshkey.c -Wall -W -I/home/lostrace/include -c -o snarf-sshkey.o

clean:
	-rm -f remote-ps snarf-sshkey *.o
