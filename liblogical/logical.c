
#include <stdint.h>
#include <errno.h>

#include <physical.h>

#include "logical.h"
#include "log_ia32.h"

// request a new handle. returns NULL on error
logical_handle logical_new_handle()
{
	logical_handle h;

	h = malloc(sizeof(struct logical_handle_data));
	if(h) {
		h->arch = arch_none;
	}

	return h;
}

// associate a logical handle with a physical memory source and a memory-architecture
int logical_handle_associate(logical_handle h, physical_handle p, enum architecture arch)
{
	if(!h)
		return -EBADR;
	
	if(arch == arch_none)
		return -EBADRQC;

	if(h->arch != arch_none)
		return -EINVAL;

	h->phy = p;
	switch(arch) {
		case arch_none:
			// will never happen, only prevents compiler-warning
			break;
		default:
			return -EBADRQC;
			break;
		case arch_ia32:
			h->arch = arch_ia32;

			h->finish			= log_ia32_finish;
			h->logical_to_physical		= log_ia32_logical_to_physical;
			h->set_new_pagedirectory	= log_ia32_set_new_pagedirectory;
			h->is_pagedir_fast		= log_ia32_is_pagedir_fast;
			h->is_pagedir_probability	= log_ia32_is_pagedir_probability;

			if(log_ia32_init(h)) {
				h->arch = arch_none;
				return -EINVAL;
			}
			break;
	}

	return 0;
}

// release a handle
int logical_handle_release(logical_handle h)
{
	if(!h)
		return -EBADR;

	h->finish(h);
	free(h);
	return 0;
}

// for all logical adressing, we need a pagedirectory with information, how to address
// set new pagedir will use a pagedir provided in a buffer.
// depending on the architecture it may load secondary and tertiary information from the
// physical source.
// the data in pagedir is not touched and not used after logical_set_new_pagedirectory().
int logical_set_new_pagedirectory(logical_handle h, void* pagedir)
{
	if(h && (h->arch != arch_none)) {
		return h->set_new_pagedirectory(h, pagedir);
	} else
		return -EBADR;
}

// load_new_pagedir will load a page from the physical source and use this
// the pagedir is copied into a buffer and this is used until the next association with
// a new pagedir.
int logical_load_new_pagedir(logical_handle h, addr_t pagedir_pageno)
{
	void* pagedir;
	int r;

	if(h && (h->arch != arch_none)) {
		pagedir = malloc(h->phy->pagesize);
		if(!pagedir)
			return -ENOMEM;

		r = physical_read_page(h->phy, pagedir_pageno, pagedir);
		if(r) {
			free(pagedir);
			return r;
		}
		r = h->set_new_pagedirectory(h, pagedir);
		free(pagedir);
		return r;
	} else
		return -EBADR;
}

// gives a quick guess, if (1) the _physical_ page is a page-directory or not (0)
// < 0 on error
int logical_is_pagedir_fast(logical_handle h, addr_t physical_pageno)
{
	if(h && (h->arch != arch_none)) {
		return h->is_pagedir_fast(h, physical_pageno);
	} else
		return -EBADR;
}

// gives a probability if (1.00) the page is a page-directory or not (0.00)
// -1 on error
float logical_is_pagedir_probability(logical_handle h, void* page)
{
	if(h && (h->arch != arch_none)) {
		return h->is_pagedir_probability(h, page);
	} else
		return -1;
}

// on paging fault (logical address is not represented by a physical address),
// all the following functions return a -EFAULT

// transform a logical address into a physical address
// if no pagedir was set, returns a -EDESTADDRREQ
int logical_to_physical(logical_handle h, addr_t log_adr, addr_t* physical_adr)
{
	if(h && (h->arch != arch_none)) {
		return h->logical_to_physical(h, log_adr, physical_adr);
	} else
		return -EBADR;
}

// read from logical address
int logical_read(logical_handle h, addr_t adr, void* buf, size_t len)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->logical_to_physical(h, adr, &ladr))
			return -EFAULT;
		else
			return physical_read(h->phy, ladr, buf, len);
	} else
		return -EBADR;
}

// write to logical address
int logical_write(logical_handle h, addr_t adr, void* buf, size_t len)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->logical_to_physical(h, adr, &ladr))
			return -EFAULT;
		else
			return physical_write(h->phy, ladr, buf, len);
	} else
		return -EBADR;
}

// read a logically addressed page
int logical_read_page(logical_handle h, addr_t pagenum, void* buf)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->logical_to_physical(h, pagenum * h->phy->pagesize, &ladr))
			return -EFAULT;
		else {
			ladr = ladr / h->phy->pagesize;
			return physical_read_page(h->phy, ladr, buf);
		}
	} else
		return -EBADR;
}

// write a logically addressed page
int logical_write_page(logical_handle h, addr_t pagenum, void* buf)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->logical_to_physical(h, pagenum * h->phy->pagesize, &ladr))
			return -EFAULT;
		else {
			ladr = ladr / h->phy->pagesize;
			return physical_write_page(h->phy, ladr, buf);
		}
	} else
		return -EBADR;
}

