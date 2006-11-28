/*  $Id$
 *  vim: fdm=marker
 *
 *  remote-ps : ps on a remote linux-ia32-box via firewire
 *
 *  Copyright (C) 2006
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

#include <physical.h>
#include <linear.h>
#include <endian_swap.h>

#include "term.h"

char pagedir[4096];

#define NODE_OFFSET     0xffc0

void dump_page(FILE* f,uint32_t pn, char* page)
{{{
	uint32_t addr;
	int i,j;
	char c;
	fprintf(f, "dump page %x\n", pn);

	addr = pn * 4096;
	for(i = 0; i<4096; i+=16) {
		fprintf(f, "page 0x%05x, addr 0x%08x: ", pn, addr+i);
		for(j = 0; j < 16; j++) {
			fprintf(f, "%02hhx ", (page+i)[j]);
			if((j+1)%4 == 0 && j)
				fputc(' ',f);
		}
		fprintf(f,"  |  ");
		for(j = 0; j < 16; j++) {
			c = (page+i)[j];
			if(c < 0x20)
				c = '.';
			if(c >= 0x7f)
				c = '.';
			fputc(c,f);
		}
		fprintf(f,"\n");
	}
}}}

///////////////////////////////////////////////////////////////////////////////
// on linux, the max environment and argument is limited by PAGESIZE * MAX_ARG_PAGES
// MAX_ARG_PAGES is 32 (statically defined in include/linux/binfmts.h)
#define MAX_ARG_PAGES 32
#define STRING_SEEK_START(p) { if(*p) {while(*p) p--; p++;} }
int proc_info(linear_handle h, int *arg_c, char ***arg_v, int *env_c, char ***env_v, char **bin)
{{{
	static char stack[4096*MAX_ARG_PAGES];
	addr_t pn;
	addr_t stack_bottom_page;
	int valid_page;

	char *p;
	int i;

	char *rargv0p;
	char *largv0p;
	char **argv;
	int argc;
	char **largv;

	char *envv0p;
	int envc;
	char **lenvv;

	// seek stack-bottom
	pn = 0xbffff;
	valid_page = 0;
	while(!valid_page && pn >= 0xbf000) {
		if(linear_to_physical(h, pn*4096, &stack_bottom_page)) {
			// page is not mapped
			pn--;
		} else {
			// found stack bottom
			valid_page = 1;
		}
	}
	if(!valid_page) {
		// there was no stack...
		// may be a kernel-thread. so this MAY be no error
		printf("(no stack found)");
		*bin = NULL;
		*arg_c = *env_c = 0;
		*arg_v = *env_v = NULL;
		return 1;
	}
	stack_bottom_page = pn;

	// obtain stack
	if(linear_read_page(h, stack_bottom_page, stack+4096*(MAX_ARG_PAGES-1))) {
		// the stack bottom was not readable...
		printf("(stackbottom (@0x%08llx) unreadable.)", stack_bottom_page);
		*bin = NULL;
		return 0;
	}
	// now read the following pages
	bzero(stack,4096*(MAX_ARG_PAGES-1));
	// page "0" was read above.
	for(i = 1; i < MAX_ARG_PAGES; i++)
		linear_read_page(h, stack_bottom_page - i, stack + 4096 * (MAX_ARG_PAGES-(i+1)));

for(i = 0; i < MAX_ARG_PAGES; i++)
	dump_page(stdout, stack_bottom_page - (MAX_ARG_PAGES-1) + i, stack+4096*(i));

	// find name of binary
	p = stack + 4096*MAX_ARG_PAGES - 6;
	STRING_SEEK_START(p);
	*bin = p;

	// now walk backwards until we find two consequtive NUL-chars.
	// this marks the beginning of args, env and name of binary.
	// point to the first non-NUL-char, that is argv[0]
	while (*(p-1) || *(p-2))
		p--;
	// this is the local address of argv[0]
	largv0p = p;
	// this is the remote address of argv[0]
	rargv0p = (char*)((uint32_t)(p - stack) + (((uint32_t)stack_bottom_page - MAX_ARG_PAGES + 1) * 4096));
#ifdef __BIG_ENDIAN__
	rargv0p = (char*)endian_swap32((uint32_t)rargv0p);
#endif

	// now find the address of argv[0] in the stack;
	// that would be argv[] then.
	// we need to search the LAST hit on the stack, as the program itself
	// may have pushed a copy of the address onto the stack.
	p = stack; argv = NULL;
	while(p) {
		argv = (char**)p;
		p = memmem(p+1, MAX_ARG_PAGES * 4096 - (p-stack), &rargv0p, 4);
	}
	if(!argv) {
		printf("(did not find argv[] on stack of \"%s\")", *bin);
		return 0;
	}
	// argc is stored right before argv[]
	argc = *((uint32_t*)(argv - 1));
#ifdef __BIG_ENDIAN__
	argc = endian_swap32(argc);
#endif
	if(argc > 32768) {
		// oops... linux does not allow more than 32768 args...
		printf("(more than 32768 args for \"%s\")", *bin);
		return 0;
	}
	if(argc < 1) {
		// oops... a program never can have <1 argument,
		// as argv[1] is the string by which it was execve()'d
		printf("(no args or negative argc for \"%s\")", *bin);
		return 0;
	}

	// now that we do have argc, we know how many arguments are real arguments, even if they contain an '=' (like "--foo=bar"). we could
	// use the argv and envv, but some application modify or delete these. so we will parse the list and create our own vectors.

	// create largv
	largv = malloc(sizeof(char*) * (argc+1));
	largv[0] = largv0p;
	largv[argc] = NULL;
	p = largv0p;
	for(i = 1; i < argc; i++) {
		// seek end of string
		while(*p && (p < ((*bin) - 1)))
			p++;
		// go to next
		p++;
		if(p >= *bin-1) {
			// argc was too big?!
			free(largv);
			printf("(argc > found arguments)");
			return 0;
		}
		largv[i] = p;
	}
	p = p + strlen(p) + 1;
	envv0p = p;

	// count env-vars
	envc = 0;
	while(p < (*bin - 1)) {
		envc++;
		while(*p)
			p++;
		p++;
	}
	// and create envv
	lenvv = malloc(sizeof(char*) * (envc+1));
	lenvv[0] = envv0p;
	lenvv[envc] = NULL;
	p = envv0p;
	for(i = 1; i < envc; i++) {
		// seek end of string
		while(*p && (p < (*bin - 1)))
			p++;
		// go to next
		p++;
		if(p < *bin)
			lenvv[i] = p;
	}

	*arg_c = argc;
	*arg_v = largv;

	*env_c = envc;
	*env_v = lenvv;

	return 1;
}}}
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char**argv)
{{{
	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;
	addr_t pn;
	char page[4096];
	float prob;

	// create and associate a physical source to /dev/mem
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
	if(argc != 2) {
		printf("please give targets nodeid\n");
		return -1;
	}
	phy_data.ieee1394.raw1394handle = raw1394_new_handle();
        if(!phy_data.ieee1394.raw1394handle) {
                printf("failed to open raw1394\n");
                return -1;
        }
	if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0)) {
		printf("raw1394 failed to set port\n");
		return -4;
	}
	phy_data.ieee1394.raw1394target = atoi(argv[1]) + NODE_OFFSET;
	printf("using target %d\n", phy_data.ieee1394.raw1394target - NODE_OFFSET);
	printf("associating physical source with raw1394\n"); fflush(stdout);
	if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}
	// associate linear
	printf("new lin handle..\n"); fflush(stdout);
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -5;
	}

	// search all pages for pagedirs
	// then, for each found, print process name
	for( pn = 0; pn < 0x80000; pn++ ) {
		if((pn%0x100) == 0) {
			printf("0x%05llx\r", pn);
			fflush(stdout);
		}
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				printf("0x%05llx @%0.3f%% ", pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("\nloading pagedir failed\n");
					continue;
				}

				int argc, envc;
				char **argv, **envv;
				char *bin;
				int i;

				if(!proc_info(lin, &argc, &argv, &envc, &envv, &bin)) {
					printf(" "TERM_RED"FAIL"TERM_RESET": %s\n", bin);
				} else {
					if(argc>0) {
						printf(" %40s\t", bin); fflush(stdout);

						for(i = 0; i<argc; i++) {
							printf("%s ", argv[i]); fflush(stdout);
						}
						putchar('\n');
						for(i = 0; i<envc; i++) {
							printf("\t\t\t\tENV(%d) %p", i, envv[i]);
							fflush(stdout);
							printf("%s\n", envv[i]);
						}
					} else {
						putchar('\n');
					}
				}
			}
		}
	}

	// release handles
	printf("rel lin handle..\n"); fflush(stdout);
	linear_handle_release(lin);
	printf("rel phy handle..\n"); fflush(stdout);
	physical_handle_release(phy);
	
	// exit 
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	return 0;
}}}

