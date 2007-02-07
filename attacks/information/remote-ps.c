/*  $Id$
 *  vim: fdm=marker
 *
 *  remote-ps : ps on a remote linux-ia32-box via firewire
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
// required for memmem:
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <elf.h>
#include <libelf.h>

#include <physical.h>
#include <linear.h>
#include <endian_swap.h>

#include "proc_info.h"
#include "term.h"

enum memsource {
	SOURCE_UNDEFINED,
	SOURCE_MEMDUMP,
	SOURCE_IEEE1394
};

struct addr_list {
	struct addr_list *next;
	addr_t adr;
	int count;
};

char pagedir[4096];
int dump_process = 0;
static struct addr_list *addr_list_head = NULL;

#define NODE_OFFSET     0xffc0

void dump_page(FILE *f, uint32_t pn, char* page)
{{{
	uint32_t addr;
	int i,j;
	char c;
	fprintf(f, "dump page %x\n", pn);

	addr = pn * 4096;
	for(i = 0; i<4096; i+=16) {
		fprintf(f, "page 0x%05x, addr 0x%08x: ", pn, addr+i);
		for(j = 0; j < 16; j++) {
			fprintf(f, "%02hhx ", (page+i)[j]);
			if((j+1)%4 == 0 && j)
				fputc(' ', f);
		}
		fprintf(f, "  |  ");
		for(j = 0; j < 16; j++) {
			c = (page+i)[j];
			if(c < 0x20)
				c = '.';
			if(c >= 0x7f)
				c = '.';
			fputc(c, f);
		}
		fprintf(f, "\n");
	}
}}}

int test_and_remember(addr_t padr)
{{{
	struct addr_list *el;

	if(!addr_list_head) {
		addr_list_head = malloc(sizeof(struct addr_list));
		addr_list_head->next = NULL;
		addr_list_head->adr = padr;
		addr_list_head->count = 1;
		el = addr_list_head;
	} else {
		el = addr_list_head;
		while(el) {
			if(el->adr == padr) {
				el->count++;
				break;
			} else {
				// element does not match
				if(el->next) {
					// test next element
					el = el->next;
				} else {
					// end of list. append new element.
					el->next = malloc(sizeof(struct addr_list));
					el = el->next;
					el->next = NULL;
					el->adr = padr;
					el->count = 1;
					break;
				}
			}
		}
	}

	return (el->count - 1);
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

void do_analyse_process(linear_handle lin, char *bin)
{{{
	char *bin2 = NULL;
	char fname[120];
	FILE *pfile = NULL;

	static int pid = 0;
	addr_t lpn;
	char page[4096];

	// create the dump-file for this process
	if(dump_process) {
		bin2 = strdup(bin);
		snprintf(fname, 119, "processes/%03d-%s", pid, basename(bin2));
		printf("dumping this process to %s\n", fname);
		pfile = fopen(fname, "w");
	}

	// scan full userspace
	for(lpn = 0; lpn < 0xc0000; lpn++) {
		// if we dump the process, read full page and dump it, for all pages
		if(dump_process) {
			if(0 == linear_read_page(lin, lpn, page))
				dump_page(pfile, lpn, page);
			else
				continue;
		}
		// if process dumping, page is already loaded; else load first 4 bytes
		// and test for ELF
		if(dump_process || (0 == linear_read(lin, lpn << 12, page, 4))) {
			if(is_elf(page)) {
				if(!dump_process) {
					// page was not yet loaded
					linear_read_page(lin, lpn, page);
				}
				Elf32_Ehdr *hdr;
				int correct_byteorder;
				uint16_t i;
				addr_t padr;

			      linear_to_physical(lin, lpn << 12, &padr);
				printf("\tELF at 0x%08llx (phys 0x%08llx): ", lpn << 12, padr);

				hdr = (Elf32_Ehdr*) page;
				switch (hdr->e_ident[EI_CLASS]) {
					case ELFCLASS32:
						break;
					default:
						printf("ELF is not 32bit. not analysing this one.\n");
						continue;
				}
				switch (hdr->e_ident[EI_DATA]) {
					case ELFDATA2LSB:
						printf("LSB, ");
#if __BYTE_ORDER == __BIG_ENDIAN
						correct_byteorder = 1;
#else
						correct_byteorder = 0;
#endif
						break;
					case ELFDATA2MSB:
						printf("MSB, ");
#if __BYTE_ORDER == __BIG_ENDIAN
						correct_byteorder = 0;
#else
						correct_byteorder = 1;
#endif
						break;
					default:
						printf("unknown byteorder. not analysing this one.\n"); 
						continue;
				}

				i = hdr->e_type;
				//printf("%x ", i);
				if(correct_byteorder) {
					i = endian_swap16(i);
				}
				//printf("%x ", i);
				switch (i) {
					case ET_NONE: printf("unknown type. "); break;
					case ET_REL:  printf("relocatable file. "); break;
					case ET_EXEC: printf("executable file. "); break;
					case ET_DYN:  
						      linear_to_physical(lin, lpn << 12, &padr);
						      printf("shared object, seen %d times ",
						             test_and_remember(padr));
						      break;
					case ET_CORE: printf("core file. "); break;
					default:      printf("unknown filetype. "); break;
				}
				putchar('\n');
			}//ENDIF is elf
		}
	}

	// release
	pid++;
	if(pfile)
		fclose(pfile);
	if(bin2)
		free(bin2);
}}}

void usage(char* argv0)
{{{
	printf( "%s [-] <"TERM_YELLOW"-n nodeid"TERM_RESET"|"TERM_YELLOW"-f filename"TERM_RESET">\n"
		"\tprint a process list of\n"
		"\t\t* the host connected via firewire with the given nodeid\n"
		"\t\t* the memory dump in the given file\n"
		"\t-e : print full environment for each process\n"
		"\t-i : show some info on each mapped ELF\n"
		"\t-d : dump virtual address space of each process (only if -i)\n",
		argv0);
}}}

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
	char *filename = NULL;
	int nodeid = 0;
	int analyse_process = 0;
	int print_environment = 0;

	while( -1 != (c = getopt(argc, argv, "n:f:ied"))) {
		switch (c) {
			case 'n':
				// phys.source: ieee1394
				memsource = SOURCE_IEEE1394;
				nodeid = strtoll(optarg, &p, 10);
				if((p&&(*p)) || (nodeid > 63) || (nodeid < 0)) {
					printf("invalid nodeid. nodeid should be >=0 and <64.\n");
					usage(argv[0]);
					return -2;
				}
				break;
			case 'f':
				// phys.source: file
				memsource = SOURCE_MEMDUMP;
				filename = optarg;
				break;
			case 'i':
				// give info on mapped ELFs for each process
				analyse_process = 1;
				break;
			case 'e':
				// print full environment for each process
				print_environment = 1;
				break;
			case 'd':
				// dump each process's virtual address space to a file
				printf("dumping all processes to file\n");
				dump_process = 1;
				mkdir("processes", 0700);
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
		phy_data.ieee1394.raw1394target_nid = nodeid + NODE_OFFSET;
		phy_data.ieee1394.raw1394target_guid = 0;
		printf("using target %d\n", phy_data.ieee1394.raw1394target_nid - NODE_OFFSET);
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

	// search all pages in lowmem for pagedirs
	// then, for each found, print process name
	for( pn = 0; pn < 0x40000; pn++ ) {
		if((pn%0x100) == 0) {
			printf("0x%05llx\r", pn);
			fflush(stdout);
		}
		if((c = linear_is_pagedir_fast(lin, pn))) {
			if(c < 0) {
				printf("\n\n" TERM_RED "failed to read page 0x%05llx. aborting." TERM_RESET "\n", pn);
				break;
			}
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
					if(analyse_process && bin)
						do_analyse_process(lin, bin);
				} else {
					if(argc>0) {
						printf(" %40s\t", bin); fflush(stdout);

						// print argv
						for(i = 0; i<argc; i++) {
							printf("%s ", argv[i]); fflush(stdout);
						}
						putchar('\n');
						// print envv
						if(print_environment) {
							for(i = 0; i<envc; i++) {
								printf("\t\t\t\tENV(%03d) @[%p] ", i, envv[i]);
								fflush(stdout);
								printf("\"%s\"\n", envv[i]);
							}
						}
						if(analyse_process)
							do_analyse_process(lin, bin);
					} else {
						putchar('\n');
					}
					if(envv)
						free(envv);
					if(argv)
						free(argv);
				}
			}
		}
	}

	if(analyse_process) {
		struct addr_list *el;

		//print list of mapped libs with count
		printf("\n\nfound the following libs:\n");
		while(addr_list_head) {
			el = addr_list_head;
			if(el->count > 5)
				printf(TERM_BLUE);
			printf("\t0x%08llx, count %02d"TERM_RESET"\n", el->adr, el->count);
			addr_list_head = el->next;
			free(el);
		}
	}

	// release handles
	linear_handle_release(lin);
	raw1394_destroy_handle(phy_data.ieee1394.raw1394handle);
	physical_handle_release(phy);
	
	return 0;
}}}

