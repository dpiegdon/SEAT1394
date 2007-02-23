/*  $Id: remote-ps.c 375 2007-02-12 00:11:46Z lostrace $
 *  vim: fdm=marker
 *
 *  remote-ps : ps on a remote linux-ia32-box via firewire
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

#include <physical.h>
#include <linear.h>
#include <ia32/ia32_paging.h>

enum memsource {
	SOURCE_UNDEFINED,
	SOURCE_MEMDUMP,
	SOURCE_IEEE1394
};

char pagedir[4096];
int dump_process = 0;

#define NODE_OFFSET     0xffc0

void dump_page(FILE *f, uint32_t pn, char* page)
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
				fputc(' ', f);
		}
		fprintf(f, "  |  ");
		for(j = 0; j < 16; j++) {
			c = (page+i)[j];
			if(c < 0x20)
				c = '.';
			if(c >= 0x7f)
				c = '.';
			fputc(c, f);
		}
		fprintf(f, "\n");
	}
}}}

void usage(char* argv0)
{{{
	printf( "%s <-f filename>\n",
		argv0);
}}}

int main(int argc, char**argv)
{{{
	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;

	addr_t pn;
	char page[4096];
	int c;

	enum memsource memsource = SOURCE_UNDEFINED;
	char *filename = NULL;

	while( -1 != (c = getopt(argc, argv, "f:"))) {
		switch (c) {
			case 'f':
				// phys.source: file
				memsource = SOURCE_MEMDUMP;
				filename = optarg;
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
	if( memsource == SOURCE_MEMDUMP ) {
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

	// associate linear
	printf("new lin handle..\n"); fflush(stdout);
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -5;
	}

	mkdir("pages/", 00700);
	// search all pages in lowmem for pagedirs
	// then, for each found, print process name
	for( pn = 0; pn < 0x40000; pn++ ) {
		if(!linear_is_pagedir_fast(lin, pn))
			continue;
		if((pn%0x100) == 0) {
			printf("0x%05llx           \r", pn);
			fflush(stdout);
		}
		if(physical_read_page(phy, pn, page)) {
			printf("EOF?\n");
			return 0;
		}
		if(!linear_set_new_pagedirectory(lin, page)) {
			for(c = 0; c < 1024; c++)
				if(((struct pagedir_simple*)page)[c].P)
					break;

			if(c == 1024) {
				continue;
			} else {
				float prob;
				prob = linear_is_pagedir_probability(lin, page);
				if(prob < 0.03)
					printf("prob %f. too small.\r",prob);
				else {
					char filename[20];
					int fd;
					printf("okay, prob %f. dumping to %s\n", prob, filename);
					snprintf(filename, 20, "pages/p%05llx", pn);
					fd = open(filename, O_RDWR | O_CREAT, 00700);
					if(fd < 0) {
						printf("failed to open \"%s\"!\n", filename);
						exit(-1);
					}
					write(fd, page, 4096);
					close(fd);
				}
			}
		}
	}

	// release handles
	linear_handle_release(lin);
	physical_handle_release(phy);
	
	return 0;
}}}

