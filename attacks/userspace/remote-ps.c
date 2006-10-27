/*  $Id$
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <physical.h>
#include <linear.h>

char pagedir[4096];

#define NODE_OFFSET     0xffc0

void print_argv(linear_handle h)
{
	addr_t pn;
	addr_t padr;
	char* page;
	page = malloc(4096);
	int valid_page = 0;

	pn = 0xbffff;

	while(!valid_page && pn >= 0xbf000) {
		if(linear_to_physical(h, pn*4096, &padr)) {
			// page is not mapped
		} else {
			// found stack bottom
			valid_page = 1;

			printf("  |  lin. page 0x%05x -> phys. page 0x%05x", (uint32_t)pn, (uint32_t)padr >> 12);
			if(linear_read_page(h, pn, page))
				printf("UNREADABLE PAGE");
			else {
				char* p = page + 4096 - 1;

				// in stack: seek process name
				while(*p == 0)
					p--;
				while(*p != 0)
					p--;
				p++;
				printf("  |  process: %s", p);
			}
		}
		pn--;
	}

	free(page);
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
	for( pn = 0; pn < 0x80000; pn++ ) {
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
				print_argv(lin);
				printf("\n");
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

