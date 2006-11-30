/*  $Id$
 *  memory dumper
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

#define NODE_OFFSET     0xffc0

int main(int argc, char**argv)
{
	int dumpfd;
	physical_handle phy;
	union physical_type_data phy_data;
	addr_t pn;
	char page[4096];

	// create and associate a physical source to firewire device
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
	if(argc != 3) {
		printf("%s: <nodeid of target> <dumpfile>\n", argv[0]);
		return -1;
	}
	// get raw1394 handle
	phy_data.ieee1394.raw1394handle = raw1394_new_handle();
	if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0)) {
		printf("raw1394 failed to set port\n");
		return -4;
	}
	// and associate
	phy_data.ieee1394.raw1394target = atoi(argv[1]) + NODE_OFFSET;
	printf("using target %d\n", phy_data.ieee1394.raw1394target - NODE_OFFSET);
	printf("associating physical source with raw1394\n"); fflush(stdout);
	if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}

	// open dumpfile
	dumpfd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600);

	// dump the memory
	for( pn = 0; pn < 0xfffff; pn++ ) {
		if(!(pn % 0x100)) {
			printf("page 0x%05x\r", (uint32_t)pn);
			fflush(stdout);
		}
		if(physical_read_page(phy, pn, page)) {
			printf("failed to read page 0x%05x\n", (uint32_t)pn);
			break;
		}
		write(dumpfd, page, 4096);
	}

	// release handles
	printf("rel phy handle..\n"); fflush(stdout);
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	physical_handle_release(phy);
	
	close(dumpfd);

	return 0;
}

