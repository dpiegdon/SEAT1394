
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


int main(int argc, char**argv)
{
	int memfd;
	int dumpfd;

	phys_handle phy;
	union phys_type_data phy_data;
	logical_handle log;

	int pagedir_pageno;
	addr_t pn;

	// create and associate a physical source to /dev/mem
	phy = phys_new_handle();
	if(!phy) {
		printf("physical handle is null\n");
		return -2;
	}
	memfd = open("/dev/mem", O_RDWR | O_LARGEFILE);
//	memfd = open("/home/datenhalde/fwire/2/memdump", O_RDWR | O_LARGEFILE | O_SYNC);
	if(memfd < 0) {
		printf("failed to open /dev/mem\n");
		return -3;
	}

	phy_data.filedescriptor.fd = memfd;

	if(phys_handle_associate(phy, phys_filedescriptor, &phy_data, 4096)) {
		printf("phys_handle_associate() failed\n");
		return -4;
	}

	printf("new log handle..\n"); fflush(stdout);
	log = logical_new_handle();
	printf("assoc log handle..\n"); fflush(stdout);
	if(logical_handle_associate(log, phy, arch_ia32)) {
		printf("failed to associate log ia32!\n");
		return -5;
	}

#define PAGEDIR 0x063a0
#define LOGICAL 0xbfa919e0

	if(!logical_is_pagedir_fast(log, PAGEDIR)) {
		printf("is not a pagedir\n");
		return -6;
	}

	printf("loading pagedir at page %p\n", PAGEDIR);
	if(logical_load_new_pagedir(log, PAGEDIR)) {
		printf("failed.\n");
		return -7;
	}
	printf("ok\n");

	uint32_t foo = 0x30303030;
	if(logical_read(log, LOGICAL, &foo, sizeof(foo))) {
		printf("*log fails to log_read\n");
	} else
		printf("*log is 0x%08x\n", foo);

	printf("trying to overwrite...\n");
	foo = 0xdeadbeef;
	int bar;
	if(bar = logical_write(log, LOGICAL, &foo, sizeof(foo))) {
		printf("overwrite failed: %d (%s)\n",bar, strerror(errno));
		
	}

	printf("rel log handle..\n"); fflush(stdout);
	logical_handle_release(log);
	printf("rel phy handle..\n"); fflush(stdout);
	phys_handle_release(phy);
	
	// exit 
	close(dumpfd);
	close(memfd);
	return 0;
}

