/*  $Id$
 *  vim: fdm=marker
 *
 *  show-stack: dump a stack page of a process
 *
 *  Copyright (C) 2006,2007
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

// continuous dumping until signal
volatile int continuous = 0;

#define NODE_OFFSET     0xffc0

void de_continue(int __attribute__ ((__unused__)) signal)
{{{
	continuous = 0;
}}}

void dump_page_wide(FILE *f, uint32_t pn, char* page, char* diffpage)
{{{
	uint32_t addr;
	int i,j;
	fprintf(f, "dump page %x\n", pn);
#define COLUMNCOUNT 32
	// 56
//#define HEX_ONLY

	addr = pn * 4096;
	for(i = 0; i<4096; i+=COLUMNCOUNT) {
		fprintf(f, "(p 0x%05x) 0x%08x: ", pn, addr+i);
		for(j = 0; j < COLUMNCOUNT; j++) {
			if( (page+i+j) >= page+4096 )
				break;
			if( (page+i)[j] != (diffpage+i)[j])
				printf(TERM_RED);
			fprintf(f, "%02hhx ", (page+i)[j]);
			if( (page+i)[j] != (diffpage+i)[j])
				printf(TERM_RESET);

			if((j+1)%4 == 0 && j)
				fputc(' ', f);
		}
#ifndef HEX_ONLY
		char c;
		fprintf(f, "  |  ");
		for(j = 0; j < COLUMNCOUNT; j++) {
			if( (page+i+j) >= page+4096 )
				break;

			c = (page+i)[j];
			if(c < 0x20)
				c = '.';
			if(c >= 0x7f)
				c = '.';
			fputc(c, f);
		}
#endif
		fprintf(f, "\n");
	}
}}}

void show_stack(linear_handle lin)
{{{
	int c = 0;
	addr_t pn;
	addr_t foo;
	int valid_page;
	char*page;
	char page1[4096];
	char page2[4096];

	// seek stack-bottom
	page = page1;
	pn = 0xbffff;
	valid_page = 0;
	while(!valid_page && pn >= 0xbf000) {
		if(linear_to_physical(lin, pn*4096, &foo)) {
			// page is not mapped
			pn--;
		} else {
			// found stack bottom
			valid_page = 1;
		}
	}

	signal(SIGINT, de_continue);

	while((c != 'q') && (c != EOF)) {
		if(page == page1)
			page = page2;
		else
			page = page1;

		if(0 == linear_read_page(lin, pn, page)) {
			if(page == page1)
				dump_page_wide(stdout, pn, page, page2);
			else
				dump_page_wide(stdout, pn, page, page1);
		} else
			printf("page 0x%0llx unmapped\n", pn);
		if(!continuous) {
			c = getchar();
			switch(c) {
				case 'u':
					getchar();
					pn--;
					break;
				case 'd':
					getchar();
					pn++;
					break;
				case 'c':
					getchar();
					continuous = 1;
					c = 0xff;
					break;
				case 'g':
					getchar();
					scanf("0x%06llx", &pn);
					break;
				case 0xa:
					// nothing
					break;
				default:
					// nothing
					getchar();
					break;
			}
		} else {
			usleep(500000);
		}
	}

}}}

void usage(char* argv0)
{{{
	printf( "%s [-] <"TERM_YELLOW"-n nodeid"TERM_RESET"|"TERM_YELLOW"-f filename"TERM_RESET"> -b <binary>\n"
		"\ncontinually dump the stack of the first process matching the -b <binary>\n"
		"\t\t* the host connected via firewire with the given nodeid\n"
		"\t\t* the memory dump in the given file\n",
		argv0);
}}}

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

	enum memsource memsource = SOURCE_UNDEFINED;
	char *filename = NULL;
	int nodeid = 0;

	while( -1 != (c = getopt(argc, argv, "n:f:b:"))) {
		switch (c) {
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
		phy_data.ieee1394.raw1394target = nodeid + NODE_OFFSET;
		printf("using target %d\n", phy_data.ieee1394.raw1394target - NODE_OFFSET);
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

	// associate linear
	printf("new lin handle..\n"); fflush(stdout);
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -5;
	}

	// search all pages in lowmem for pagedirs
	// then, for each found, print process name
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
						printf("\tmatching!\n");

						show_stack(lin);

						break;
					}
				}
			}
		}
	}

	// release handles
	linear_handle_release(lin);
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	physical_handle_release(phy);
	
	return 0;
}}}

