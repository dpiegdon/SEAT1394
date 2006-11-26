
.PHONY: all clean

all:	remote-ps snarf-sshkey dump-sshkey

openssh/openbsd-compat/libopenbsd-compat.a:
	make -C openssh/openbsd-compat/ libopenbsd-compat.a

openssh/libssh.a:
	make -C openssh/ libssh.a

openssh/entropy.o: openssh/entropy.c
	make -C openssh/ entropy.o

remote-ps: remote-ps.c
	gcc remote-ps.c -Wall -W -lphysical -llinear -L/home/lostrace/lib -I/home/lostrace/include -o remote-ps

snarf-sshkey: openssh/openbsd-compat/libopenbsd-compat.a openssh/libssh.a openssh/entropy.o snarf-sshkey.o sshkey-sanitychecks.o
	gcc snarf-sshkey.o sshkey-sanitychecks.o openssh/entropy.o						\
		-lm -lresolv -lcrypto -lcrypt -lutil -lz -lnsl   -lssh -lopenbsd-compat   -lphysical -llinear	\
		-L ./openssh/ -L ./openssh/openbsd-compat/ -L${HOME}/lib					\
		-o snarf-sshkey

dump-sshkey: openssh/openbsd-compat/libopenbsd-compat.a openssh/libssh.a openssh/entropy.o dump-sshkey.o sshkey-sanitychecks.o
	gcc dump-sshkey.o sshkey-sanitychecks.o openssh/entropy.o				\
		-lm -lresolv -lcrypto -lcrypt -lutil -lz -lnsl   -lssh -lopenbsd-compat		\
		-L ./openssh/ -L ./openssh/openbsd-compat/ -L${HOME}/lib			\
		-o dump-sshkey

sshkey-sanitychecks.o: sshkey-sanitychecks.h sshkey-sanitychecks.c term.h
	gcc sshkey-sanitychecks.c -Wall -W -DUSETERM -c -o sshkey-sanitychecks.o

snarf-sshkey.o: snarf-sshkey.c ssh-authfile.h term.h
	gcc snarf-sshkey.c -Wall -W -I/home/lostrace/include -DUSETERM -c -o snarf-sshkey.o

dump-sshkey.o: dump-sshkey.c ssh-authfile.h term.h
	gcc dump-sshkey.c -Wall -W -DUSETERM -c -o dump-sshkey.o

clean:
	-rm -f remote-ps snarf-sshkey *.o
