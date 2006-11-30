
.PHONY: all clean

all:	remote-ps snarf-sshkey

clean:
	-rm -f remote-ps snarf-sshkey debug-sshkey *.o





remote-ps: remote-ps.o proc_info.o
	gcc remote-ps.o proc_info.o -lphysical -llinear -L/home/lostrace/lib   -DUSETERM -o remote-ps

snarf-sshkey: openssh/openbsd-compat/libopenbsd-compat.a openssh/libssh.a openssh/entropy.o snarf-sshkey.o sshkey-sanitychecks.o proc_info.o
	gcc snarf-sshkey.o proc_info.o sshkey-sanitychecks.o openssh/entropy.o						\
		-lm -lresolv -lcrypto -lcrypt -lutil -lz -lnsl   -lssh -lopenbsd-compat   -lphysical -llinear	\
		-L ./openssh/ -L ./openssh/openbsd-compat/ -L${HOME}/lib					\
		-o snarf-sshkey

debug-sshkey: openssh/openbsd-compat/libopenbsd-compat.a openssh/libssh.a openssh/entropy.o debug-sshkey.o sshkey-sanitychecks.o
	gcc debug-sshkey.o sshkey-sanitychecks.o openssh/entropy.o				\
		-lm -lresolv -lcrypto -lcrypt -lutil -lz -lnsl   -lssh -lopenbsd-compat		\
		-L ./openssh/ -L ./openssh/openbsd-compat/ -L${HOME}/lib			\
		-o debug-sshkey





openssh/openbsd-compat/libopenbsd-compat.a:
	make -C openssh/openbsd-compat/ libopenbsd-compat.a

openssh/libssh.a:
	make -C openssh/ libssh.a

openssh/entropy.o: openssh/entropy.c
	make -C openssh/ entropy.o



remote-ps.o: remote-ps.c
	gcc remote-ps.c -Wall -W -I/home/lostrace/include -c -o remote-ps.o

proc_info.o: proc_info.c
	gcc proc_info.c -Wall -W -I/home/lostrace/include -c -o proc_info.o

sshkey-sanitychecks.o: sshkey-sanitychecks.h sshkey-sanitychecks.c term.h
	gcc sshkey-sanitychecks.c -Wall -W -DUSETERM -c -o sshkey-sanitychecks.o

snarf-sshkey.o: snarf-sshkey.c ssh-authfile.h term.h
	gcc snarf-sshkey.c -Wall -W -I/home/lostrace/include -DUSETERM -c -o snarf-sshkey.o

debug-sshkey.o: debug-sshkey.c ssh-authfile.h term.h
	gcc debug-sshkey.c -Wall -W -DUSETERM -c -o debug-sshkey.o

