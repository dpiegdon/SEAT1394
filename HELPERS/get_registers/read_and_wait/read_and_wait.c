#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "term.h"
#include "get_registers.h"

#include <ia32/ia32_control_registers.h>
#include <ia32/ia32_gpr.h>
#include <ia32/ia32_paging.h>
#include <ia32/ia32_segment.h>

#define PAGEDIRECTORY_SIZE 4096

int main()
{
	FILE* r;
	union registers regs;
	uint32_t pagebase;
	int c;

	r = fopen("/proc/registerdump", "rb");
	fread(&regs.dump, sizeof(regs.dump), 1, r);
	fclose(r);

	printf("got:\n"
		"\tCR0: %08x\n"
		"\tCR3: %08x\n"
		"\tCR4: %08x\n\n"

		"\tESP: %08x\n"
		"\tEBP: %08x\n"
		"\tESI: %08x\n"
		"\tEDI: %08x\n\n"

		"\tCS:  %04x i:%c%04d, rpl:%2d \t\tlinux standard GDT:\n"
		"\tDS:  %04x i:%c%04d, rpl:%2d \t\t |\t0x00cf9a000000ffff        /* 0x60 kernel 4GB code at 0x00000000 */\n"
		"\tES:  %04x i:%c%04d, rpl:%2d \t\t |\t0x00cf92000000ffff        /* 0x68 kernel 4GB data at 0x00000000 */\n"
		"\tFS:  %04x i:%c%04d, rpl:%2d \t\t |\t0x00cffa000000ffff        /* 0x73 user 4GB code at 0x00000000 */\n"
		"\tGS:  %04x i:%c%04d, rpl:%2d \t\t |\t0x00cff2000000ffff        /* 0x7b user 4GB data at 0x00000000 */\n"
		"\tSS:  %04x i:%c%04d, rpl:%2d \t\t |\t0x0000000000000000        /* 0x33 TLS entry 1 */\n"
 
		"\n\n",
		regs.dump.cr0,
		regs.dump.cr3,
		regs.dump.cr4,

		regs.dump.esp,
		regs.dump.ebp,
		regs.dump.esi,
		regs.dump.edi,

		regs.dump.cs, regs.parsed.cs.TI ? 'l' : 'g', regs.parsed.cs.index, regs.parsed.cs.RPL,
		regs.dump.ds, regs.parsed.ds.TI ? 'l' : 'g', regs.parsed.ds.index, regs.parsed.ds.RPL,
		regs.dump.es, regs.parsed.es.TI ? 'l' : 'g', regs.parsed.es.index, regs.parsed.es.RPL,
		regs.dump.fs, regs.parsed.fs.TI ? 'l' : 'g', regs.parsed.fs.index, regs.parsed.fs.RPL,
		regs.dump.gs, regs.parsed.gs.TI ? 'l' : 'g', regs.parsed.gs.index, regs.parsed.gs.RPL,
		regs.dump.ss, regs.parsed.ss.TI ? 'l' : 'g', regs.parsed.ss.index, regs.parsed.ss.RPL
		);


	// eval:
	// CR0
	printf("CR0:\n");
		if(regs.parsed.cr0.PE) printf("\tPE:  protection enable\n");
		if(regs.parsed.cr0.MP) printf("\tMP:  monitor coprocessor\n");
		if(regs.parsed.cr0.EM) printf("\tEM:  emulation\n");
		if(regs.parsed.cr0.TS) printf("\tTS:  task switched\n");
		if(regs.parsed.cr0.ET) printf("\tET:  extension type\n");
		if(regs.parsed.cr0.NE) printf("\tNE:  numeric error\n");
		if(regs.parsed.cr0.WP) printf("\tWP:  write protect\n");
		if(regs.parsed.cr0.AM) printf("\tAM:  alignment mask\n");
		if(regs.parsed.cr0.NW) printf("\tNW:  not write-through\n");
		if(regs.parsed.cr0.CD) printf("\tCD:  cache disable\n");
		if(regs.parsed.cr0.PG) printf("\tPG:  paging\n");
	printf("\n");

	pagebase = regs.parsed.cr3.pagedir_base << 12;
	printf("CR3:\n");
		printf("\tpagedir base:  %08x\n", pagebase);
		if(regs.parsed.cr3.PWT) printf("\tPWT: page-level writes transparent\n");
		if(regs.parsed.cr3.PCD) printf("\tPCD: page-level cache disable\n");
	printf("\n");

	printf("CR4:\n");
		if(regs.parsed.cr4.VME) printf("\tVME: virtual-8086 mode extension\n");
		if(regs.parsed.cr4.PVI) printf("\tPVI: protected-mode virtual interrupts\n");
		if(regs.parsed.cr4.TSD) printf("\tTSD: time stamp disable\n");
		if(regs.parsed.cr4.DE) printf("\tDE:  debugging extensions\n");
		if(regs.parsed.cr4.PSE) printf("\tPSE: page size extension\n");
		if(regs.parsed.cr4.PAE) printf("\tPAE: physical address extension\n");
		if(regs.parsed.cr4.MCE) printf("\tMCE: machine-check enable\n");
		if(regs.parsed.cr4.PGE) printf("\tPGE: page global enable\n");
		if(regs.parsed.cr4.PCE) printf("\tPCE: performance-monitoring counter enable\n");
		if(regs.parsed.cr4.OSFXSR) printf("\tOSFXSR:     Operating System Support for FXSAVE and FXRSTOR instructions\n");
		if(regs.parsed.cr4.OSXMMEXCPT) printf("\tOSXMMEXCPT: Operating System Support for Unmasked SIMD Floating-Point Exceptions\n");
	printf("\n");

	printf("shall i read and analyse the data at %p? [yN]\n", pagebase);
	c = getchar();

	if(c == 'Y' || c == 'y') {
		FILE* mem;
		FILE* dumpfile;
		union pagedir_entry pagedir[1024];

		// get page-directory (CR3)
		{
			// open physical memory device
			mem = fopen("/dev/mem", "rb");
			if(!mem) {
				printf("failed to open \"/dev/mem\". missing permissions?\naborting.\n");
				return 0;
			}
			// seek page
			fseek(mem, pagebase, SEEK_SET);
			// read page to buffer
			fread(pagedir, sizeof(union pagedir_entry), 1024, mem);

			// open dumpfile
			dumpfile = fopen("dump/pagedir", "w");
			if(!dumpfile) {
				printf("failed to create \"dump/pagedir\". missing permissions?\naborting.\n");
				return 0;
			}
			// write buffer to dumpfile
			fwrite(pagedir, sizeof(union pagedir_entry), 1024, dumpfile);
			// close dumpfile
			fclose(dumpfile);
		}
		// analyse entries
		{
			int i,j;

			for(i = 1; i < 1024; i++) {
				if(pagedir[i].simple.P) {

					// if PS == 0, we have a pagedir-entry that points to a page-table. save it, too.
					if(0 == pagedir[i].simple.PS ) {
						char fname[255];
						struct pagetable_entry pagetable[1024];

						pagebase = pagedir[i].table.base_adr << 12;
						printf("PDE %4x:  " TERM_BLUE "4k-table" TERM_RESET", phys %08x,                     %s,        %s, PCD:%s, PWT:%s, %s, R/W:%s",
							i,
							pagebase,
							pagedir[i].table.G   ? TERM_RED "global" TERM_RESET	: " local",				// G: Global
							pagedir[i].table.A   ? "accs'd"				: TERM_MAGENTA "no acc" TERM_RESET,	// A: Accessed
							pagedir[i].table.PCD ? TERM_BLUE "1" TERM_RESET 	: "0",					// PCD: Cache Disabled
							pagedir[i].table.PWT ? TERM_RED "1" TERM_RESET 		: "0",					// PWT: Write Trough
							pagedir[i].table.U_S ? " user" 				: TERM_RED "super" TERM_RESET,		// User/Supervisor
							pagedir[i].table.R_W ? "rw" 				: TERM_MAGENTA "ro" TERM_RESET		// Read/Write
						      );

						// seek pagetable in physical memory
						fseek(mem, pagebase, SEEK_SET);
						// copy page table to buffer
						fread(pagetable, sizeof(struct pagetable_entry), 1024, mem);

						// open dumpfile
						snprintf(fname, 254, "dump/pagetab%08x", i * 1024 * 4);
						dumpfile = fopen(fname, "w");
						// write ptable
						fwrite(pagetable, sizeof(struct pagetable_entry), 1024, dumpfile);
						// close dumpfile
						fclose(dumpfile);

						printf("\t\t\t\tSTORED tab%08x\n", i * 1024 * 4);
/*
						// now parse this pagetable, as well.
						for( j = 0; j < 1024; j++ ) {
							// if page is present, show it.
							if(pagetable[j].P) {
								printf(" PTE %4x:  4K-page, phys %08x, PAT:%s, avail:0x%3x, %s, %s, %s, PCD:%s, PWT:%s, %s, R/W:%s, logical adr %08x\n",
									j,
									pagetable[j].base_adr << 12,
									pagetable[j].PAT ? "1"					: "0",
									pagetable[j].avail0,
									pagetable[j].G   ? TERM_RED "global" TERM_RESET 	: " local",
									pagetable[j].D   ? TERM_YELLOW "dirty" TERM_RESET	: "clean",
									pagetable[j].A   ? "accs'd"				: TERM_MAGENTA "no acc" TERM_RESET,
									pagetable[j].PCD ? TERM_BLUE "1" TERM_RESET		: "0",
									pagetable[j].PWT ? TERM_RED "1" TERM_RESET		: "0",
									pagetable[j].U_S ? " user"				: TERM_RED "super" TERM_RESET,
									pagetable[j].R_W ? "rw"					: TERM_MAGENTA "ro" TERM_RESET,
									i * 1024 * 1024 * 4 + j * 1024 * 4
								      );
							}
						}
*/
					} else {
						printf("PDE %4x:   4M-page, phys %08x, PAT:%s, avail:0x%3x, %s, %s, %s, PCD:%s, PWT:%s, %s, R/W:%s, logical adr %08x\n",
							i,
							pagedir[i].page.base_adr << 22,
							pagedir[i].page.PAT ? "1"			 	: "0",					// PAT
							pagedir[i].page.avail0,
							pagedir[i].page.G   ? TERM_RED "global" TERM_RESET	: " local",				// G: Global
							pagedir[i].page.D   ? TERM_YELLOW "dirty" TERM_RESET	: "clean",				// D: Dirty
							pagedir[i].page.A   ? "accs'd" 				: TERM_MAGENTA "no acc" TERM_RESET,	// A: Accessed
							pagedir[i].page.PCD ? TERM_BLUE "1" TERM_RESET 		: "0",					// PCD: Cache Disabled
							pagedir[i].page.PWT ? TERM_RED "1" TERM_RESET 		: "0",					// PWT: Write Trough
							pagedir[i].page.U_S ? " user" 				: TERM_RED "super" TERM_RESET,		// User/Supervisor
							pagedir[i].page.R_W ? "rw" 				: TERM_MAGENTA "ro" TERM_RESET,		// Read/Write
							i * 1024 * 1024 * 4
						      );
						// no. we do have a pagedir-entry for a 4MB-page
					}
				}
			}
		}
		{
			volatile uint32_t foo;
			foo = 0x172342ff;
			while(1) {
				r = fopen("/proc/registerdump", "rb");
				fread(&regs.dump, sizeof(regs.dump), 1, r);
				fclose(r);

				pagebase = regs.parsed.cr3.pagedir_base << 12;
				printf("\tpagedir:  %08x\n", pagebase >> 12);
				printf("*(%p) == 0x%8x\n", &foo, foo);
				sleep(1);
			}
		}
		fclose(mem);

		printf("dump done (in \"pagedir-dump\")\n");
	}

	return 0;
}
