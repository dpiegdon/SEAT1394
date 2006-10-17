
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
	pn = 0xbfc00;

	//while(pn < (4*1024*1024)) {
	while(pn < 0xc0000) {
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

void print_stack(linear_handle h)
{
	static char* page;
	addr_t pn;
	int e;
	addr_t upper;
	addr_t padr;

	page = malloc(4096);

	printf("\tstack of this process:\n");

	// seek from 0xC0000000 backwards, until the stack-bottom is found
	// (there is always some space between 0xc0000000 and the real stack
	// that is located somewhere 0xBFxxxxxx
	pn = ( 0xc0000000 / h->phy->pagesize );

	e = -EFAULT;
	while(e == -EFAULT) {
		pn--;
		if(linear_to_physical(h, pn*4096, &padr))
			printf("faulty:\n");
		printf("seek_upper linear address 0x%08x maps to 0x%08x\n", (uint32_t)pn*4096, (uint32_t)padr);
		e = linear_read_page(h, pn, page);
	};
	printf("found upper stack-bound at page 0x%05x\n", (uint32_t)pn);
	upper = pn+1;
	while(e != -EFAULT) {
		pn--;
//		printf("seek_lower linear address 0x%08x\n", (uint32_t)pn*4096);
		e = linear_read_page(h, pn, page);
//		printf("%d\n", e);
	};
	printf("found upper stack-bound at page 0x%05x\n", (uint32_t)pn);
	// now we are at top-of-stack
	// so dump it
	while(pn<upper) {
		if(!linear_read_page(h, pn, page)) {
			printf("dump page %x to %x\n", (uint32_t)pn, (uint32_t)upper);
			dump_page(pn, page);
		}
		pn++;
	}
	free(page);
}


int main(/*int argc, char**argv*/)
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
	addr_t padr;

	// search all pages for pagedirs
	// then, for each found, print stack of process
	// page 0x60000: 1.5GB physical
	for( pn = 0; pn < 0x60000; pn++ ) {
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			printf("page 0x%05x prob: %f \n", (uint32_t)pn, prob);
			if(prob > 0.1) {
				printf("pagedir:\n");
				//dump_page(pn, page);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("loading pagedir failed\n");
					continue;
				}
				//linear_to_physical(lin, 0xc0000000, &padr);
				//printf("0xC0000000 -> 0x%08x\n", (uint32_t)padr);
				//print_stack(lin);
				print_linear(lin);
				//return 0;
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

