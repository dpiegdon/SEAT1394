#   $Id$
#   information gathering
# 
#   Copyright (C) 2006,2007
#   losTrace aka "David R. Piegdon"
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2, as
#   published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

.PHONY: all clean

CC=gcc

PREFIX?=$(HOME)

CFLAGS += -I$(PREFIX)/include -O9 -W -Wall -fPIC -finline-functions -ffast-math -funroll-all-loops -Wpointer-arith -DUSETERM
LDFLAGS+= -L$(PREFIX)/lib -s

all:	dump-memory remote-ps debug-sshkey snarf-sshkey test-all-pagedirs show-stack

clean:
	-rm -f dump-memory remote-ps snarf-sshkey snarf-sshkey-static debug-sshkey *.o test-all-pagedirs show-stack

dump-memory: LDFLAGS += -lphysical -llinear
dump-memory: dump-memory.o

show-stack: LDFLAGS += -lphysical -llinear
show-stack: show-stack.o proc_info.o

remote-ps: LDFLAGS += -lphysical -llinear
remote-ps: remote-ps.o proc_info.o

snarf-sshkey: LDFLAGS += -lphysical -llinear -lcrypto
snarf-sshkey: snarf-sshkey.o sshkey-sanitychecks.o proc_info.o ssh-authfile.o

snarf-sshkey-static: LDFLAGS += -static -lphysical -llinear -lcrypto -lraw1394 -ldl -lbz2
snarf-sshkey-static: snarf-sshkey.o sshkey-sanitychecks.o proc_info.o ssh-authfile.o
	$(CC) snarf-sshkey.o sshkey-sanitychecks.o proc_info.o ssh-authfile.o $(LDFLAGS) -o $@

debug-sshkey: LDFLAGS += -lcrypto
debug-sshkey: debug-sshkey.o sshkey-sanitychecks.o ssh-authfile.o

test-all-pagedirs: LDFLAGS += -lphysical -llinear
test-all-pagedirs: test-all-pagedirs.o


dump-memory.o: dump-memory.c

remote-ps.o: remote-ps.c

proc_info.o: proc_info.c

sshkey-sanitychecks.o: sshkey-sanitychecks.h sshkey-sanitychecks.c term.h

snarf-sshkey.o: snarf-sshkey.c ssh-authfile.h term.h

debug-sshkey.o: debug-sshkey.c ssh-authfile.h term.h

ssh-authfile.o: ssh-authfile.h ssh-authfile.c

test-all-pagedirs.o: test-all-pagedirs.c

