
#include <errno.h>
#include <string.h>

#include <ia32/ia32_paging.h>
#include <physical.h>
#include <endian_swap.h>

#include "linear.h"
#include "simple_ncd.h"
#include "lin_ia32_sample_pagedirs.h"

#define PDE(number) (	(union pagedir_entry*)				\
				(h->data.ia32.pagedir) + (number)	\
			)

////////////////////////////////////////////////////////////////////////////////
// functions to check for signatures of a pagedir

static int pagedirtest_fast_linux3G1G(struct linear_handle_data* h, addr_t physical_pageno)
{
	union pagedir_entry pde;

	// on ia32-linux with a __PAGE_OFFSET of 0xc0000000, the linear adr 0xc0000000 will resolve to phys 0x0.
	// check this. 0xc0000000 is PDE#0x300. should be a 4M-page and not a pointer to a pagetable.
	// but check this possibility, too (?)
	// if not, this is not a linux-PDE with __PAGE_OFFSET 0xc0000000 (that is a linux with 3G/1G memlayout)
	physical_read(h->phy, physical_pageno * h->phy->pagesize + 0x300*sizeof(union pagedir_entry), &pde, sizeof(pde));
#ifdef __BIG_ENDIAN__
	pde.raw = endian_swap32(pde.raw);
#endif
	if(pde.simple.P) {
		if(pde.simple.PS) {
			// 4M-page: this entry should be global, superuser (U_S == 0) and the phys page located at 0x0
			if(pde.page.G && !pde.page.U_S && (pde.page.base_adr == 0)) {
				return 1; // looks good.
			}
/*		} else {
			struct pagetable_entry pte;
			// 4k-pagetable: this should not be... but lets check anyway.
			// load the PTE#0 and check if this is ok and resolves to 0x0
			physical_read(h->phy, pde.table.base_adr + 0*sizeof(struct pagetable_entry), &pte, sizeof(pte));
			if(pte.base_adr == 0) {
				return 1; // possibly...
			}
*/		}
	}

	return 0;
}

static float pagedirtest_prob_linux3G1G(struct linear_handle_data* h, void* page)
{
	float ncd = 2;
	// test NCD against sample page
	// for bzip2: blocksize, workfactor, bzverbosity
	// 	defaults may be used when omitted
	ncd = simple_ncd(page, h->phy->pagesize, lin_ia32_samplepagedir_linux3G1G, 4096);

	if(ncd > 1.)
		ncd = 1.;
	return 1. - ncd;
}

// TODO:
//	Linux2G2G		(__PAGE_OFFSET = 0x80000000 ?)
//	Windows (XP, ...)
//	(Free,Open,Net)-BSD
//	...

////////////////////////////////////////////////////////////////////////////////
// linear interface

int lin_ia32_init(struct linear_handle_data* h)
{
	int pn;

	// enforce a _typical_ pagesize of 4096 bytes
	if(h->phy->pagesize != 4096)
		return -1;

	// reset pagedir and pagetable-cache
	h->data.ia32.pagedir = NULL;
	for( pn = 0; pn < 1024; pn++) {
		h->data.ia32.pagetable_dir[pn] = NULL;
	}

	return 0;
}

int lin_ia32_finish(struct linear_handle_data* h)
{
	int pn;

	for(pn=0; pn<1024; pn++) {
		if(h->data.ia32.pagetable_dir[pn]) {
			free(h->data.ia32.pagetable_dir[pn]);
			h->data.ia32.pagetable_dir[pn] = NULL;
		}
	}
	return 0;
}

int lin_ia32_is_pagedir_fast(struct linear_handle_data* h, addr_t physical_pageno)
{
	return (
			pagedirtest_fast_linux3G1G(h, physical_pageno)
	       );
}

float lin_ia32_is_pagedir_probability(struct linear_handle_data* h, void* page)
{
	return (
			// MAX(...)
			pagedirtest_prob_linux3G1G(h, page)
	       );
}

int lin_ia32_linear_to_physical(struct linear_handle_data* h, addr_t lin_adr, addr_t* physical_adr)
{
	union pagedir_entry *pde;
	struct pagetable_entry *pt;
	struct pagetable_entry *pte;
	union physical_page_mapping padr;

	// physical address space can be no more than 4GB
	// (FIXME: there are physical workarounds like PAE and PSE to increase to 16GB)
	padr.adr = (lin_adr & 0xffffffff);

	// get matching PDE
	pde = PDE(padr.pde_mapping.pde_entry);

	if(pde->simple.P) {
		if(pde->simple.PS) {
			// 4M-page
			//          phys page base            +  page offset
			*physical_adr = (pde->page.base_adr << 22) + (padr.pde_mapping.page_offset);
			return 0;
		} else {
			// 4k-pagetable
			// get matching pagetable from cache
			pt = h->data.ia32.pagetable_dir[padr.pte_mapping.pde_entry];
			// and get matching PTE-entry in PT
			pte = pt + padr.pte_mapping.pte_entry;
			if(pte->P) {
				*physical_adr = (pte->base_adr << 12) + (padr.pte_mapping.page_offset);
				return 0;
			} else {
				return -EFAULT;
			}
		}
	} else {
		return -EFAULT;
	}
}

int lin_ia32_set_new_pagedirectory(struct linear_handle_data* h, void* pagedir)
{
	int pn;					// table entry number
	union pagedir_entry	*pde;

	// free current buffers, if existing
	if(h->data.ia32.pagedir)
		free(h->data.ia32.pagedir);
	for(pn=0; pn<1024; pn++)
		if(h->data.ia32.pagetable_dir[pn]) {
			free(h->data.ia32.pagetable_dir[pn]);
			h->data.ia32.pagetable_dir[pn] = NULL;
		}

	// copy pagedirectory into buffer
	h->data.ia32.pagedir = malloc( h->phy->pagesize );
	memcpy(h->data.ia32.pagedir, pagedir, h->phy->pagesize);
	// copy all pagetables to buffer
	for(pn=0; pn<1024; pn++) {
		pde = PDE(pn);
#ifdef __BIG_ENDIAN__
		// correct byte-order on the fly
		pde->raw = endian_swap32(pde->raw);
		// and once and for all.
		PDE(pn)->raw = pde->raw;
#endif
		if( (pde->simple.P == 1) && (pde->simple.PS == 0) ) {
			// pagedir entry is present and a pointer to a pagetable,
			// it points to a pagetable. cache this one, too.
			h->data.ia32.pagetable_dir[pn] = malloc( h->phy->pagesize );
			if(h->data.ia32.pagetable_dir[pn] == NULL) {
				// free all loaded PTs and return an error

				return -ENOMEM;
			}
			if(physical_read_page(h->phy, pde->table.base_adr, h->data.ia32.pagetable_dir[pn])) {
				// error.
				// free all loaded PTs and return an error

				return errno;
			} else {
#ifdef __BIG_ENDIAN__
				int ptc;
				uint32_t* pte;
				// correct byte-order on the fly in all pagetables
				pte = h->data.ia32.pagetable_dir[pn];
				for(ptc = 0; ptc < 1024; ptc++) {
					*pte = endian_swap32(*pte);
					pte++;
				}
#endif
			}
		}
	}

	return 0;
}

