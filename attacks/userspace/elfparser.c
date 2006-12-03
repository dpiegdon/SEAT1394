/*  $Id$
 *  vim: fdm=marker
 *
 *  elfparser : parse information from mapped ELF-files
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

/* {{{ includes */
#define _LARGEFILE64_SOURCE
// required for memmem:
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <physical.h>
#include <linear.h>
#include <endian_swap.h>

#include "proc_info.h"
#include "term.h"

#define NODE_OFFSET     0xffc0
/* }}} */
#include <elf.h>
#include <libelf.h>

char pagedir[4096];

addr_t seek_mapped_page(linear_handle h, addr_t start_page, int max_distance, int direction)
{{{
	addr_t pn;
	addr_t foo;

	if(direction == 1) {
		for(pn = start_page; pn <= start_page + max_distance; pn++)
			if(0 == linear_to_physical(h, pn * h->phy->pagesize, &foo))
				return pn;
	} else if(direction == -1) {
		for(pn = start_page; pn >= start_page - max_distance; pn--)
			if(0 == linear_to_physical(h, pn * h->phy->pagesize, &foo))
				return pn;
	} else {
		return -1;
	}

	return -1;
}}}

addr_t seek_unmapped_page(linear_handle h, addr_t start_page, int max_distance, int direction)
{{{
	addr_t pn;
	addr_t foo;

	if(direction == 1) {
		for(pn = start_page; pn <= start_page + max_distance; pn++)
			if(0 != linear_to_physical(h, pn * h->phy->pagesize, &foo))
				return pn;
	} else if(direction == -1) {
		for(pn = start_page; pn >= start_page - max_distance; pn--)
			if(0 != linear_to_physical(h, pn * h->phy->pagesize, &foo))
				return pn;
	} else {
		return -1;
	}

	return -1;
}}}

int is_elf(char *c)
{{{
	if(   (c[0] == ELFMAG0)
	   && (c[1] == ELFMAG1)
	   && (c[2] == ELFMAG2)
	   && (c[3] == ELFMAG3) )
		return 1;
	else
		return 0;
}}}

void elf_parser(linear_handle h)
{
	addr_t p;
	addr_t q;
	int i;

	char page[4096];
	int correct_byteorder;

	char *code;
	Elf *elf;
	Elf32_Ehdr *hdr;
	int elflen;

	{{{ // get first mapped area, if its an ELF
		p = seek_mapped_page(h, 0x08000, 0x05000, 1);
		printf("first mapped page at 0x%05llx\n", p);
		if(p == (uint64_t)-1) {
			printf("failed to find mapped page\n");
			return;
		}
		// test the page for ELF header
		linear_read_page(h, p, page);

		if(!is_elf(page)) {
			printf("mapped area is not in ELF\n");
			return;
		}
		printf("mapped area is elf\n");
		q = seek_unmapped_page(h, p, 0x100, 1);
		if(q == (uint64_t)-1) {
			printf("failed to find end of mapped area\n");
			return;
		}
		q--;
		printf("last mapped page at 0x%05llx\n", q);
		elflen = (q-p+1) * 4096;

		code = calloc(4096, (q-p)+1);

		for(i = 0; (p+i) <= q; i++) {
			linear_read_page(h, p+i, code + i*4096);
		}
	}}}
	{{{ // get elf handle for block and print some info
		char*info;
		size_t info_len;
		unsigned int i;

		hdr = (Elf32_Ehdr*)code;
		elf = elf_memory(code, elflen);
		info = elf_getident(elf, &info_len);
		printf("elf info [len %d]: "TERM_BLUE, info_len);
		for(i = 0; i<info_len; i++) {
			printf("%02X ", info[i]);
		}
		printf(TERM_RESET"\n");

		if(code[EI_CLASS] != ELFCLASS32) {
			printf("is not a 32bit elf!\n");
			goto exit;
		}

		switch (hdr->e_ident[EI_CLASS]) {
			case ELFCLASS32: printf("32 bit "); break;
			case ELFCLASS64: printf("64 bit "); break;
			default: printf("[unknown bits] "); break;
		}

		switch (hdr->e_ident[EI_DATA]) {
			case ELFDATA2LSB:
				printf("LSB ");
#ifdef __BIG_ENDIAN__
				correct_byteorder = 1;
#else
				correct_byteorder = 0;
#endif
				break;
			case ELFDATA2MSB:
				printf("MSB ");
#ifdef __BIG_ENDIAN__
				correct_byteorder = 0;
#else
				correct_byteorder = 1;
#endif
				break;
			default:
				printf("[unknown byteorder] "); 
				goto exit;
				break;
		}

		switch (hdr->e_ident[EI_OSABI]) {
			//case ELFOSABI_NONE:       printf("Same as ELFOSABI_SYSV "); break;
			case ELFOSABI_SYSV:       printf("UNIX System V ABI. "); break;
			case ELFOSABI_HPUX:       printf("HP-UX ABI. "); break;
			case ELFOSABI_NETBSD:     printf("NetBSD ABI. "); break;
			case ELFOSABI_LINUX:      printf("Linux ABI. "); break;
			case ELFOSABI_SOLARIS:    printf("Solaris ABI. "); break;
			case ELFOSABI_IRIX:       printf("IRIX ABI. "); break;
			case ELFOSABI_FREEBSD:    printf("FreeBSD ABI. "); break;
			case ELFOSABI_TRU64:      printf("TRU64 UNIX ABI. "); break;
			case ELFOSABI_ARM:        printf("ARM architecture ABI. "); break;
			case ELFOSABI_STANDALONE: printf("Stand-alone (embedded) ABI. "); break;
			default:                  printf("[unknown ABI] "); break;
		}
		putchar('\n');

		i = hdr->e_type;
		printf("%x ", i);
		if(correct_byteorder) {
			i = endian_swap32(i);
		}
		printf("%x\n", i);
		switch (i) {
                        case ET_NONE: printf("An unknown type. "); break;
                        case ET_REL:  printf("A relocatable file. "); break;
                        case ET_EXEC: printf("An executable file. "); break;
                        case ET_DYN:  printf("A shared object. "); break;
                        case ET_CORE: printf("A core file. "); break;
			default:      printf("unknown filetype. "); break;
		}
		putchar('\n');
	}}}
exit:
	elf_end(elf);
	free(code);
}

void usage(char* argv0)
{{{
	printf( "%s <"TERM_YELLOW"-n nodeid"TERM_RESET"|"TERM_YELLOW"-f filename"TERM_RESET">\n"
		"\tprint a process list of\n"
		"\t\t* the host connected via firewire with the given nodeid\n"
		"\t\t* the memory dump in the given file\n",
		argv0);
}}}

enum memsource {
	SOURCE_UNDEFINED,
	SOURCE_MEMDUMP,
	SOURCE_IEEE1394
};

int main(int argc, char**argv)
{{{
	physical_handle phy;
	union physical_type_data phy_data;
	linear_handle lin;

	addr_t pn;
	char page[4096];
	float prob;
	int c;
	char *p;

	enum memsource memsource = SOURCE_UNDEFINED;
	char *filename;
	int nodeid;

	while( -1 != (c = getopt(argc, argv, "n:f:"))) {
		switch (c) {
			case 'n':
				memsource = SOURCE_IEEE1394;
				nodeid = strtoll(optarg, &p, 10);
				if((p&&(*p)) || (nodeid > 63) || (nodeid < 0)) {
					printf("invalid nodeid. nodeid should be >=0 and <64.\n");
					usage(argv[0]);
					return -2;
				}
				break;
			case 'f':
				memsource = SOURCE_MEMDUMP;
				filename = optarg;
				break;
			default:
				usage(argv[0]);
				return -2;
				break;
		}
	}
	if(c != -1) {
		printf("too many arguments? (\"%s\")\n", argv[c]);
		usage(argv[0]);
		return -2;
	}

	// create and associate a physical source to /dev/mem
	phy = physical_new_handle();
	if(!phy)
		{ printf("physical handle is null\n"); return -2; }
	if( memsource == SOURCE_IEEE1394 ) {
		// create raw1394handle
		phy_data.ieee1394.raw1394handle = raw1394_new_handle();
		if(!phy_data.ieee1394.raw1394handle)
			{ printf("failed to open raw1394\n"); return -3; }
		// associate raw1394 to port
		if(raw1394_set_port(phy_data.ieee1394.raw1394handle, 0))
			{ printf("raw1394 failed to set port\n"); return -3; }
		// set attack target
		phy_data.ieee1394.raw1394target = nodeid + NODE_OFFSET;
		printf("using target %d\n", phy_data.ieee1394.raw1394target - NODE_OFFSET);
		if(physical_handle_associate(phy, physical_ieee1394, &phy_data, 4096)) {
			printf("physical_handle_associate() failed\n");
			return -3;
		}
	} else if( memsource == SOURCE_MEMDUMP ) {
		c = open(filename, O_RDONLY);
		if(c < 0) {
			printf("failed to open file \"%s\"\n", filename);
			return -2;
		}
		phy_data.filedescriptor.fd = c;
		if(physical_handle_associate(phy, physical_filedescriptor, &phy_data, 4096)) {
			printf("physical_handle_associate() failed\n");
			return -3;
		}
	} else {
		printf("missing memory source\n");
		usage(argv[0]);
		return -2;
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
		if((pn%0x100) == 0) {
			printf("0x%05llx\r", pn);
			fflush(stdout);
		}
		if(linear_is_pagedir_fast(lin, pn)) {
			// load page
			physical_read_page(phy, pn, page);
			prob = linear_is_pagedir_probability(lin, page);
			if(prob > 0.01) {
				printf("0x%05llx [~%0.3f] ", pn, prob);
				if(linear_set_new_pagedirectory(lin, page)) {
					printf("\nloading pagedir failed\n");
					continue;
				}

				int argc, envc;
				char **argv, **envv;
				char *bin;
				int i;

				if(!proc_info(lin, &argc, &argv, &envc, &envv, &bin)) {
					printf(" "TERM_RED"FAIL"TERM_RESET": %s\n", bin);
				} else {
					if(argc>0) {
						printf(" %40s\t", bin); fflush(stdout);

						// print argv
						for(i = 0; i<argc; i++) {
							printf("%s ", argv[i]); fflush(stdout);
						}
						putchar('\n');
						elf_parser(lin);
					} else {
						putchar('\n');
					}
				}
			}
		}
	}

	// release handles
	linear_handle_release(lin);
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	physical_handle_release(phy);
	
	return 0;
}}}

