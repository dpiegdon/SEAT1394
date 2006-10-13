
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

struct logical_arch_none_data {
	int foo;				// unused
};
struct logical_arch_ia32_data {
	// typical pagesize is 4096 bytes,
	// pagetable and pagedir all have 1024 4-byte-entries
	void** pagedir;				// buffered pagedir
	void* pagetable_dir[1024];		// buffered pagetables
};
union architecture_data {
	struct logical_arch_none_data none;
	struct logical_arch_ia32_data ia32;
};

struct logical_handle_data {
	physical_handle		phy;		// physical memory source
	enum architecture	arch;		// architecture of physical source
	union architecture_data	data;		// architecture specific data

	// architecture-specific functions:
	int(*finish)(struct logical_handle_data* h);

	int(*set_new_pagedirectory)(struct logical_handle_data* h, void* pagedir);

	int(*is_pagedir_fast)(struct logical_handle_data* h, addr_t physical_pageno);
	float(*is_pagedir_probability)(struct logical_handle_data* h, void* page);

	int(*logical_to_physical)(struct logical_handle_data* h, addr_t physical_adr, addr_t* log_adr);
};

typedef struct logical_handle_data* logical_handle;

///////////// functions

// request a new handle. returns NULL on error
logical_handle logical_new_handle();

// associate a logical handle with a physical memory source and a memory-architecture
int logical_handle_associate(logical_handle h, physical_handle p, enum architecture arch);

// release a handle
int logical_handle_release(logical_handle h);

// for all logical adressing, we need a pagedirectory with information, how to address
// set new pagedir will use a pagedir provided in a buffer.
// depending on the architecture it may load secondary and tertiary information from the
// physical source.
// the data in pagedir is not touched and not used after logical_set_new_pagedirectory().
int logical_set_new_pagedirectory(logical_handle h, void* pagedir);
// load_new_pagedir will load a page from the physical source and use this
// the pagedir is copied into a buffer and this is used until the next association with
// a new pagedir.
int logical_load_new_pagedir(logical_handle h, addr_t pagedir_pageno);

// gives a quick guess, if (1) the _physical_ page is a page-directory or not (0)
// < 0 on error
int logical_is_pagedir_fast(logical_handle h, addr_t physical_pageno);

// gives a probability if (1.00) the page is a page-directory or not (0.00)
// -1 on error
float logical_is_pagedir_probability(logical_handle h, void* page);

// on paging fault (logical address is not represented by a physical address),
// all the following functions return a -EFAULT

// transform a logical address into a physical address
// if no pagedir was set, returns a -EDESTADDRREQ
int logical_to_physical(logical_handle h, addr_t log_adr, addr_t* physical_adr);

// read from logical address
int logical_read(logical_handle h, addr_t adr, void* buf, size_t len);

// write to logical address
int logical_write(logical_handle h, addr_t adr, void* buf, size_t len);

// read a logically addressed page
int logical_read_page(logical_handle h, addr_t pagenum, void* buf);

// write a logically addressed page
int logical_write_page(logical_handle h, addr_t pagenum, void* buf);

#endif // __LOGICAL_H__

