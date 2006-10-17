
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

#define PHYSICAL_DEV_MEM
//#define MEMSOURCE "/dev/mem"
#define MEMSOURCE "/home/datenhalde/memdump"

void dump_page(uint32_t pn, char* page)
{
	uint32_t addr;
	int i;

	addr = pn * 4096;
	for(i = 0; i<4095/32; i+=32) {
		printf("page 0x%06x, addr 0x%08x: %2x %2x %2x %2x  %2x %2x %2x %2x  %2x %2x %2x %2x  %2x %2x %2x %2x  |  %c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c\n",
			pn, addr,
			(page+i)[0], (page+i)[1], (page+i)[2], (page+i)[3], (page+i)[4], (page+i)[5], (page+i)[6], (page+i)[7], (page+i)[8], (page+i)[9], (page+i)[10], (page+i)[11], (page+i)[12], (page+i)[13], (page+i)[14], (page+i)[15], (page+i)[16], (page+i)[17], (page+i)[18], (page+i)[19], (page+i)[20], (page+i)[21], (page+i)[22], (page+i)[23], (page+i)[24], (page+i)[25], (page+i)[26], (page+i)[27], (page+i)[28], (page+i)[29], (page+i)[30], (page+i)[31], (page+i)[32], (page+i)[39],
			(page+i)[0], (page+i)[1], (page+i)[2], (page+i)[3], (page+i)[4], (page+i)[5], (page+i)[6], (page+i)[7], (page+i)[8], (page+i)[9], (page+i)[10], (page+i)[11], (page+i)[12], (page+i)[13], (page+i)[14], (page+i)[15], (page+i)[16], (page+i)[17], (page+i)[18], (page+i)[19], (page+i)[20], (page+i)[21], (page+i)[22], (page+i)[23], (page+i)[24], (page+i)[25], (page+i)[26], (page+i)[27], (page+i)[28], (page+i)[29], (page+i)[30], (page+i)[31], (page+i)[32], (page+i)[39]
		      );
	}

}


void print_stack(linear_handle h)
{
	static char* page;
	addr_t pn;
	int e;

	page = malloc(4096);

	printf("\tstack of this process:\n");

	// seek from 0xC0000000 backwards, until the stack-bottom is found
	// (there is always some space between 0xc0000000 and the real stack
	// that is located somewhere 0xBFxxxxxx
	pn = ( 0xc0000000 / h->phy->pagesize );

	e = -EFAULT;
	while(e == -EFAULT) {
		pn--;
		printf("linear address 0x%08x\n", (uint32_t)pn*4096);
		e = linear_read_page(h, pn, page);
	};
	printf("found upper stack-bound\n");
	while(e != -EFAULT) {
		pn--;
		printf("linear address 0x%08x\n", (uint32_t)pn*4096);
		e = linear_read_page(h, pn, page);
		printf("%d\n", e);
	};
	printf("found lower stack-bound\n");
	// now we are at top-of-stack
	// so dump it
	while(1) {
		if(-EFAULT == linear_read_page(h, pn, page))
			break;
		dump_page(pn, page);
		pn++;
	}

}


int main(int argc, char**argv)
{
#ifdef PHYSICAL_DEV_MEM
	int memfd;
#endif
	int dumpfd;

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
	// XXX
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
	for( pn = 0; pn < 0x60000; pn++ ) {
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			printf("page %llu prob: %f \n", pn, prob);
			if(prob > 0.3) {
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("loading pagedir failed\n");
					continue;
				}
				print_stack(lin);
			}
		}
	}


	// FUNSTUFF end

	// release handles
	printf("rel lin handle..\n"); fflush(stdout);
	linear_handle_release(lin);
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

