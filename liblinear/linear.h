/*  $Id$
 *  liblinear
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

#ifndef __LOGICAL_H__
# define __LOGICAL_H__

#include <stdint.h>
#include <stdlib.h>

#include <physical.h>

enum architecture {
	arch_none		= 0,
	arch_ia32
// TODO:  (hope someone else does them... ;)
//	arch_ia64
//	arch_amd64
//	arch_ppc32
//	arch_ppc64
//	...
};

struct linear_arch_none_data {
	int foo;				// unused
};
struct linear_arch_ia32_data {
	// typical pagesize is 4096 bytes,
	// pagetable and pagedir all have 1024 4-byte-entries
	void** pagedir;				// buffered pagedir
	void* pagetable_dir[1024];		// buffered pagetables
};
union architecture_data {
	struct linear_arch_none_data none;
	struct linear_arch_ia32_data ia32;
};

struct linear_handle_data {
	physical_handle		phy;		// physical memory source
	enum architecture	arch;		// architecture of physical source
	union architecture_data	data;		// architecture specific data

	// architecture-specific functions:
	int(*finish)(struct linear_handle_data* h);

	int(*set_new_pagedirectory)(struct linear_handle_data* h, void* pagedir);

	int(*is_pagedir_fast)(struct linear_handle_data* h, addr_t physical_pageno);
	float(*is_pagedir_probability)(struct linear_handle_data* h, void* page);

	int(*linear_to_physical)(struct linear_handle_data* h, addr_t physical_adr, addr_t* log_adr);
};

typedef struct linear_handle_data* linear_handle;

///////////// functions

// request a new handle. returns NULL on error
linear_handle linear_new_handle();

// associate a linear handle with a physical memory source and a memory-architecture
int linear_handle_associate(linear_handle h, physical_handle p, enum architecture arch);

// release a handle
int linear_handle_release(linear_handle h);

// for all linear adressing, we need a pagedirectory with information, how to address.
// set_new_pagedir will use a pagedir provided in a buffer.
// depending on the architecture it may load secondary and tertiary information from the
// physical source.
// the data in pagedir is not touched and not used after linear_set_new_pagedirectory(),
// all needed data is internally buffered.
int linear_set_new_pagedirectory(linear_handle h, void* pagedir);

// load_new_pagedir will load a page from the physical source and use this page as the
// new pagedir via set_new_pagedirectory.
int linear_load_new_pagedir(linear_handle h, addr_t pagedir_pageno);

// gives a quick guess, if (1) the _physical_ page is a page-directory on this
// architecture or not (0)
// < 0 on error (especially -EBADR)
int linear_is_pagedir_fast(linear_handle h, addr_t physical_pageno);

// gives a probability if (1.00) the page is a page-directory or not (0.00)
// -1 on error
float linear_is_pagedir_probability(linear_handle h, void* page);

// on paging fault (linear address is not represented by a physical address),
// all the following functions return a -EFAULT

// transform a linear address into a physical address
// if no pagedir was set, returns a -EDESTADDRREQ
int linear_to_physical(linear_handle h, addr_t log_adr, addr_t* physical_adr);

// read from linear address: it has to be sure, that this data is inside
// a single page
int linear_read_in_page(linear_handle h, addr_t adr, void* buf, unsigned long int len);

// read from linear address
int linear_read(linear_handle h, addr_t adr, void* buf, unsigned long int len);

// write to linear address: it has to be sure, that this data is inside
// a single page
int linear_write_in_page(linear_handle h, addr_t adr, void* buf, unsigned long int len);

// write to linear address
int linear_write(linear_handle h, addr_t adr, void* buf, unsigned long int len);

// read a linearly addressed page
int linear_read_page(linear_handle h, addr_t pagenum, void* buf);

// write a linearly addressed page
int linear_write_page(linear_handle h, addr_t pagenum, void* buf);

// seek first mapped page from a given start-location in a direction (-1 or +1)
addr_t linear_seek_mapped_page(linear_handle h, addr_t start_page, int max_distance, int direction);

// seek first unmapped page from a given start-location in a direction (-1 or +1)
addr_t linear_seek_unmapped_page(linear_handle h, addr_t start_page, int max_distance, int direction);

#endif // __LOGICAL_H__

