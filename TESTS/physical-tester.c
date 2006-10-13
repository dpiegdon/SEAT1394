
#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <physical.h>




int main()
{
	int memfd;
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
	memfd = open("/dev/mem", O_RDONLY | O_LARGEFILE);
	if(memfd < 0) {
		printf("failed to open /dev/mem\n");
		return -2;
	}

	phy_data.filedescriptor.fd = memfd;

	printf("associating physical source with fd%d (pagebuf=%p)\n", memfd, pagebuf); fflush(stdout);
	if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
		printf("physical_handle_associate() failed\n");
		return -3;
	}

	printf("dumping (pagebuf=%p)\n", pagebuf); fflush(stdout);
	// read some data from 0x0...0x010000 (10 pages)
	dumpfd = open("pagedump", O_WRONLY|O_CREAT, 0600);
	if(dumpfd < 0) {
		printf("failed to open pagedump\n");
		return -4;
	}

	printf("now really dumping (pagebuf=%p)\n", pagebuf); fflush(stdout);
	for(pn = 0; pn < 10; pn++) {
		printf("\ndumping page %llu (read to %p)\n", pn, pagebuf);
		if(physical_read(phy, 4096 * pn, pagebuf, 4096)) {
			printf("failed to read page %llu\n", pn);
		}
		if(4096 != write(dumpfd, pagebuf, 4096)) {
			printf("failed to write page %llu\n", pn);
		}
	}
	
	// exit 
	close(dumpfd);
	close(memfd);
	return 0;
}

