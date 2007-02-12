/*  $Id$
 *  vim: fdm=marker
 *  memory dumper
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
#include <errno.h>

#include <physical.h>

#define NODE_OFFSET     0xffc0

void usage(char* argv0)
{{{
	printf( "%s -f <filename> -n|-g <nodeid|guid> [-s] [-c]\n"
		"\tdumps memory of IEEE1394-node <nodeid> or <guid> to <filename>\n"
		"\t-s : skip memory [0xc0000 , 0xfffff]\n"
		"\t\t(dumping this could kill a windoze)\n"
		"\t-c : continue if some pages fail to be dumped\n"
		"\t\tin this case you will have to stop me with a signal.\n",
		argv0);
}}}

int main(int argc, char**argv)
{{{
	int nodeid = -1;
	uint64_t guid = 0;
	char *filename = NULL;
	int dumpfd;
	int do_skip = 0;
	int do_cont = 0;
	char c;
	char *p;

	physical_handle phy;
	union physical_type_data phy_data;
	addr_t pn;
	char page[4096];
	int last_read_failed;
	int i;

	while( -1 != (c = getopt(argc, argv, "g:n:f:sc"))) {
		switch (c) {
			case 'g':
				if(nodeid != -1)
					printf("giving nodeid and GUID is insane. GUID will be used in any case.\n");
				if(guid != 0)
					printf("giving multiple GUIDs is insane. last will be used.\n");

				sscanf(optarg, "%016llX", &guid);
				printf("GUID set to %016llX\n", guid);
				break;
			case 'n':
				// firewire node ID
				if(nodeid != -1)
					printf("giving multiple nodeids is insane. last will be used.\n");

				nodeid = strtoll(optarg, &p, 10);
				if((p&&(*p)) || (nodeid > 63) || (nodeid < 0)) {
					nodeid = -1;
					printf("invalid nodeid. nodeid should be >=0 and <64.\n");
					usage(argv[0]);
					return -2;
				}
				break;
			case 'f':
				// filename for dump
				if(filename != NULL)
					printf("giving multiple filenames is insane. last will be used.\n");

				filename = optarg;
				break;
			case 's':
				// skip memory [0xc0000 , 0xfffff]
				do_skip = 1;
				break;
			case 'c':
				// continue if something fails
				do_cont = 1;
				break;
			default:
				usage(argv[0]);
				return -1;
				break;
		}
	}

	if( ( (guid == 0) && (nodeid == -1) ) || (filename == NULL) ) {
		printf("missing parameters\n");
		usage(argv[0]);
		return -1;
	}

	// open dumpfile
	dumpfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if(dumpfd < 0) {
		printf("failed to open \"%s\"\n", filename);
		usage(argv[0]);
		return -3;
	}


	// create and associate a physical source to firewire device
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
	// get raw1394 handle
	phy_data.ieee1394.raw1394handle = raw1394_new_handle();
	if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0)) {
		printf("raw1394 failed to set port\n");
		return -4;
	}
	// and associate
	phy_data.ieee1394.raw1394target_nid = nodeid + NODE_OFFSET;
	phy_data.ieee1394.raw1394target_guid = guid;
	printf("associating physical source with raw1394\n"); fflush(stdout);
	if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}

	last_read_failed = 0;
	// dump the memory
	for( pn = 0; pn < 0x100000; pn++ ) {
		if(pn < 0x100  &&  pn >= 0xc0  &&  do_skip) {
			if(pn == 0xc0) {
				printf("skipping [0xc0000 , 0xfffff]\n");
				for(i = 0; i < 4096; i++) {
					if(i < 256)
						page[i] = i;
					else
						page[i] = 0xFE;
				}
			}
			continue;
		}
		if(last_read_failed) {
			// last read failed
			if(physical_read_page(phy, pn, page)) {
				if((pn % 0x10) == 0) {
					printf("failed to read page 0x%05x\r", (uint32_t)pn);
				}
			} else {
				printf("page 0x%05x ok again\n", (uint32_t)pn);
				putchar('\n');
				last_read_failed = 0;
			}
		} else {
			// last read was ok
			if((pn % 0x100) == 0) {
				printf("page 0x%05x\r", (uint32_t)pn);
				fflush(stdout);
			}
			if(physical_read_page(phy, pn, page)) {
				last_read_failed = 1;
				printf("\nfailed to read page 0x%05x\n", (uint32_t)pn);
				if(!do_cont)
					return 0;
				for(i = 0; i < 4096; i++) {
					if(i < 256)
						page[i] = i;
					else
						page[i] = 0xFE;
				}
			}
		}


		write(dumpfd, page, 4096);
	}

	// release handles
	printf("rel phy handle..\n"); fflush(stdout);
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	physical_handle_release(phy);
	
	close(dumpfd);

	return 0;
}}}

