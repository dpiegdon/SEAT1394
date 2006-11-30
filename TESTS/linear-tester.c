/*  $Id$
 *  linear tester
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

#include <physical.h>
#include <linear.h>

char pagedir[4096];

//#define PHYSICAL_DEV_MEM
#define PHYSICAL_FIREWIRE
#define NODE_OFFSET     0xffc0
//#define MEMSOURCE "/dev/mem"
//#define MEMSOURCE "/home/datenhalde/memdump"
#define MEMSOURCE "qemu.memdump"

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
//			if((j+1)%4 == 0 && j)
//				putchar(' ');
		}
		printf("\n");
	}
}


void print_linear(linear_handle h)
{
	addr_t pn;
	addr_t padr;
	char* page;
	page = malloc(4096);

	//pn = 0;
	pn = 0xf7000;

	//while(pn < (4*1024*1024)) {
	while(pn < 0x100000) {
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

int main(
#ifdef PHYSICAL_FIREWIRE
		int argc, char**argv
#endif
	)
{
#ifdef PHYSICAL_DEV_MEM
	int memfd;
#endif

	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;

	// create and associate a physical source to /dev/mem
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
#ifdef PHYSICAL_DEV_MEM
	memfd = open(MEMSOURCE, O_RDONLY | O_LARGEFILE);
	if(memfd < 0) {
		printf("failed to open " MEMSOURCE "\n");
		return -3;
	}

	phy_data.filedescriptor.fd = memfd;
	if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -4;
	}
#endif
#ifdef PHYSICAL_FIREWIRE
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
#endif
	// associate linear
	printf("new lin handle..\n"); fflush(stdout);
	lin = linear_new_handle();
	printf("assoc lin handle..\n"); fflush(stdout);
	if(linear_handle_associate(lin, phy, arch_ia32)) {
		printf("failed to associate lin ia32!\n");
		return -5;
	}

	// FUNSTUFF begin

	addr_t pn;
	char page[4096];
	float prob;

	// search all pages for pagedirs
	// then, for each found, print stack of process
	// page 0x60000: 1.5GB physical
	for( pn = 0; pn < 0x80000; pn++ ) {
//		if(!(pn % 0x100))
//			printf("page 0x%05x\n", (uint32_t)pn);
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			printf("\npage 0x%05x prob: %f", (uint32_t)pn, prob);
			if(prob > 0.15) {
				//printf("pagedir:\n");
				//dump_page(pn, page);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("loading pagedir failed\n");
					continue;
				}
				print_linear(lin);
				//return 0;
			}
		}
	}


	// FUNSTUFF end

	// release handles
#ifdef PHYSICAL_DEV_MEM
	close(memfd);
#endif
#ifdef PHYSICAL_FIREWIRE
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
#endif
	printf("rel lin handle..\n"); fflush(stdout);
	linear_handle_release(lin);
	printf("rel phy handle..\n"); fflush(stdout);
	physical_handle_release(phy);
	
	return 0;
}

