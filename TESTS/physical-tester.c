/*  $Id$
 *  physical tester
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <physical.h>

#define PHYSICAL_FIREWIRE
#define NODE_OFFSET     0xffc0

int main(int argc, char**argv)
{
#ifdef PHYSICAL_DEV_MEM
	int memfd;
#endif
	physical_handle phy;
	union physical_type_data phy_data;

	int dumpfd;

	char pagebuf[4096];
	addr_t pn;

	// create and associate a physical source to /dev/mem
	printf("init (pagebuf=%p\n",pagebuf); fflush(stdout);
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -1;
	}

#ifdef PHYSICAL_DEV_MEM
	memfd = open("/dev/mem", O_RDONLY | O_LARGEFILE);
	if(memfd < 0) {
		printf("failed to open /dev/mem\n");
		return -2;
	}

	phy_data.filedescriptor.fd = memfd;
	printf("associating physical source with fd%d\n", memfd); fflush(stdout);
	if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}
#endif
#ifdef PHYSICAL_FIREWIRE
	if(argc != 2) {
		printf("missing nodeid as parameter\n");
		exit(-1);
	}

	phy_data.ieee1394.raw1394handle = raw1394_new_handle();
	if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0)) {
		printf("raw1394 failed to set port\n");
		return -4;
	}
	phy_data.ieee1394.raw1394target_nid = atoi(argv[1]) + NODE_OFFSET;
	phy_data.ieee1394.raw1394target_guid = 0;
	printf("using target %d\n", phy_data.ieee1394.raw1394target_nid - NODE_OFFSET);
	printf("associating physical source with raw1394\n"); fflush(stdout);
	if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}
#endif

	// read some data from 0x0...0x010000 (10 pages)
	dumpfd = open("pagedump", O_WRONLY|O_CREAT, 0600);
	if(dumpfd < 0) {
		printf("failed to open pagedump\n");
		return -5;
	}

	for(pn = 0; pn < 10; pn++) {
		printf("\ndumping page %llu (read to %p)\n", pn, pagebuf);
		if(physical_read(phy, 4096 * pn, pagebuf, 4096)) {
			printf("failed to read page %llu\n", pn);
//			continue;
		}
		if(4096 != write(dumpfd, pagebuf, 4096)) {
			printf("failed to write page %llu\n", pn);
		}
	}

	
	close(dumpfd);
#ifdef PHYSICAL_DEV_MEM
	close(memfd);
#endif
#ifdef PHYSICAL_FIREWIRE
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
#endif
	return 0;
}

