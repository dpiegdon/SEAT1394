
#ifndef __IA32_PAGING_H__
# define __IA32_PAGING_H__

#include <stdint.h>

struct pagedir_simple {
	unsigned int	P:1;				// present (1: present in physical memory)
	unsigned int	reserved1:6;			// don't touch
	unsigned int	PS:1;				// page size (if 0, use pagedir_4k_entry (it's a 4KB page-table),
							//            if 1, use pagedir_4M_entry (it's a 4MB page) )
	unsigned int	reserved0:24;			// don't touch
} __attribute__((__packed__));

struct pagedir_4k_entry {
	unsigned int	P:1;				// present (in phys. memory)
	unsigned int	R_W:1;				// read/write
	unsigned int	U_S:1;				// user/supervisor
	unsigned int	PWT:1;				// write-through
	unsigned int	PCD:1;				// cache disabled
	unsigned int	A:1;				// accessed
	unsigned int	reserved0:1;			// set to 0
	unsigned int	PS:1;				// page size (here must be 0: 4kbyte)
	unsigned int	G:1;				// global flag
	unsigned int	avail0:3;			// available for system programmers
	unsigned int	base_adr:20;			// page-table base address (bits 12-31, other bits are 0, thus at 4KB page-border)
} __attribute__((__packed__));

// 4M-pagedir entries only applies, if CR4.PSE or CR4.PAE!
struct pagedir_4M_entry {
	unsigned int	P:1;				// present (in phys. memory)
	unsigned int	R_W:1;				// read/write
	unsigned int	U_S:1;				// user/supervisor
	unsigned int	PWT:1;				// write-through
	unsigned int	PCD:1;				// cache disabled
	unsigned int	A:1;				// accessed
	unsigned int	D:1;				// dirty
	unsigned int	PS:1;				// page size (here must be 1: 4Mbyte)
	unsigned int	G:1;				// global flag
	unsigned int	avail0:3;			// available for system programmers
	unsigned int	PAT:1;				// page attribute table index

	//unsigned int	reserved0:9;			//
	// if PSE-36 paging mechanism, reserved0 is split into a 5-bit reserved (higher 5 bits) and 4-bit page-base-address (bits 32-35)
	unsigned int	pse36_base_adr_high:4;		// if PSE36, higher 4 bits (bits 32-35) of page-base
	unsigned int	reserved0:5;			//

	unsigned int	base_adr:10;			// page base address (bits 22-31, other bits are 0, thus at 4MB page-border)
} __attribute__((__packed__));

union pagedir_entry {
	struct pagedir_simple simple;
	struct pagedir_4k_entry table;
	struct pagedir_4M_entry page;
	uint32_t raw;
};


struct pagetable_entry {
	unsigned int	P:1;				// present (in phys. memory)
	unsigned int	R_W:1;				// read/write
	unsigned int	U_S:1;				// user/supervisor
	unsigned int	PWT:1;				// write-through
	unsigned int	PCD:1;				// cache disabled
	unsigned int	A:1;				// accessed
	unsigned int	D:1;				// dirty
	unsigned int	PAT:1;				// page attribute table index
	unsigned int	G:1;				// global flag
	unsigned int	avail0:3;			// available for system programmers
	unsigned int	base_adr:20;			// page base address (bits 12-31, other bits are 0, thus at 4KB page-border)
} __attribute__((__packed__));

union physical_page_mapping {
	struct {
		unsigned int adr:32;
	};
	struct {
		unsigned int page_offset:12;
		unsigned int pte_entry:10;
		unsigned int pde_entry:10;
	} pte_mapping;
	struct {
		unsigned int page_offset:22;
		unsigned int pde_entry:10;
	} pde_mapping;
} __attribute__((__packed__));

#endif // __IA32_PAGING_H__

