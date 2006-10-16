
#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <physical.h>
#include <logical.h>

char pagedir[4096];

#define PHYSICAL_DEV_MEM

int main(int argc, char**argv)
{
#ifdef PHYSICAL_DEV_MEM
	int memfd;
#endif
	int dumpfd;

	physical_handle phy;
	union physical_type_data phy_data;
	logical_handle log;

	// create and associate a physical source to /dev/mem
	phy = physical_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
#ifdef PHYSICAL_DEV_MEM
	memfd = open("/dev/mem", O_RDONLY | O_LARGEFILE);
//	memfd = open("/home/datenhalde/fwire/2/memdump", O_RDWR | O_LARGEFILE | O_SYNC);
	if(memfd < 0) {
		printf("failed to open /dev/mem\n");
		return -3;
	}

	phy_data.filedescriptor.fd = memfd;
	if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -4;
	}
#endif
#ifdef PHYSICAL_FIREWIRE
	// XXX
#endif
	// associate logical
	printf("new log handle..\n"); fflush(stdout);
	log = logical_new_handle();
	printf("assoc log handle..\n"); fflush(stdout);
	if(logical_handle_associate(log, phy, arch_ia32)) {
		printf("failed to associate log ia32!\n");
		return -5;
	}

	// FUNSTUFF begin

	addr_t pn;
	char page[4096];
	float prob;

	for( pn = 0; pn < 0x60000; pn++ ) {
//		if(logical_is_pagedir_fast(log, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = logical_is_pagedir_probability(log, page);
			printf("page %llu prob: %f \n", pn, prob);
//		}
	}


	// FUNSTUFF end

	// release handles
	printf("rel log handle..\n"); fflush(stdout);
	logical_handle_release(log);
	printf("rel phy handle..\n"); fflush(stdout);
	physical_handle_release(phy);
	
	// exit 
	close(dumpfd);
#ifdef PHYSICAL_DEV_MEM
	close(memfd);
#endif
#ifdef PHYSICAL_FIREWIRE
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
#endif
	return 0;
}

