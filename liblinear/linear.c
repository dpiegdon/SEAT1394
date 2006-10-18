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

#include <stdint.h>
#include <errno.h>

#include <physical.h>

#include "linear.h"
#include "lin_ia32.h"

// request a new handle. returns NULL on error
linear_handle linear_new_handle()
{
	linear_handle h;

	h = malloc(sizeof(struct linear_handle_data));
	if(h) {
		h->arch = arch_none;
	}

	return h;
}

// associate a linear handle with a physical memory source and a memory-architecture
int linear_handle_associate(linear_handle h, physical_handle p, enum architecture arch)
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

			h->finish			= lin_ia32_finish;
			h->linear_to_physical		= lin_ia32_linear_to_physical;
			h->set_new_pagedirectory	= lin_ia32_set_new_pagedirectory;
			h->is_pagedir_fast		= lin_ia32_is_pagedir_fast;
			h->is_pagedir_probability	= lin_ia32_is_pagedir_probability;

			if(lin_ia32_init(h)) {
				h->arch = arch_none;
				return -EINVAL;
			}
			break;
	}

	return 0;
}

// release a handle
int linear_handle_release(linear_handle h)
{
	if(!h)
		return -EBADR;

	h->finish(h);
	free(h);
	return 0;
}

// for all linear adressing, we need a pagedirectory with information, how to address.
// set_new_pagedir will use a pagedir provided in a buffer.
// depending on the architecture it may load secondary and tertiary information from the
// physical source.
// the data in pagedir is not touched and not used after linear_set_new_pagedirectory(),
// all needed data is internally buffered.
int linear_set_new_pagedirectory(linear_handle h, void* pagedir)
{
	if(h && (h->arch != arch_none)) {
		return h->set_new_pagedirectory(h, pagedir);
	} else
		return -EBADR;
}

// load_new_pagedir will load a page from the physical source and use this page as the
// new pagedir via set_new_pagedirectory.
int linear_load_new_pagedir(linear_handle h, addr_t pagedir_pageno)
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

// gives a quick guess, if (1) the _physical_ page is a page-directory on this
// architecture or not (0)
// < 0 on error
int linear_is_pagedir_fast(linear_handle h, addr_t physical_pageno)
{
	if(h && (h->arch != arch_none)) {
		return h->is_pagedir_fast(h, physical_pageno);
	} else
		return -EBADR;
}

// gives a probability if (1.00) the page is a page-directory or not (0.00)
// -1 on error
float linear_is_pagedir_probability(linear_handle h, void* page)
{
	if(h && (h->arch != arch_none)) {
		return h->is_pagedir_probability(h, page);
	} else
		return -1;
}

// on paging fault (linear address is not represented by a physical address),
// all the following functions return a -EFAULT

// transform a linear address into a physical address
// if no pagedir was set, returns a -EDESTADDRREQ
int linear_to_physical(linear_handle h, addr_t lin_adr, addr_t* physical_adr)
{
	if(h && (h->arch != arch_none)) {
		return h->linear_to_physical(h, lin_adr, physical_adr);
	} else
		return -EBADR;
}

// read from linear address
int linear_read(linear_handle h, addr_t adr, void* buf, size_t len)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->linear_to_physical(h, adr, &ladr))
			return -EFAULT;
		else
			return physical_read(h->phy, ladr, buf, len);
	} else
		return -EBADR;
}

// write to linear address
int linear_write(linear_handle h, addr_t adr, void* buf, size_t len)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->linear_to_physical(h, adr, &ladr))
			return -EFAULT;
		else
			return physical_write(h->phy, ladr, buf, len);
	} else
		return -EBADR;
}

// read a linearly addressed page
int linear_read_page(linear_handle h, addr_t pagenum, void* buf)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->linear_to_physical(h, pagenum * h->phy->pagesize, &ladr))
			return -EFAULT;
		else {
			ladr = ladr / h->phy->pagesize;
			return physical_read_page(h->phy, ladr, buf);
		}
	} else
		return -EBADR;
}

// write a linearly addressed page
int linear_write_page(linear_handle h, addr_t pagenum, void* buf)
{
	addr_t ladr;

	if(h && (h->arch != arch_none)) {
		if(-EFAULT == h->linear_to_physical(h, pagenum * h->phy->pagesize, &ladr))
			return -EFAULT;
		else {
			ladr = ladr / h->phy->pagesize;
			return physical_write_page(h->phy, ladr, buf);
		}
	} else
		return -EBADR;
}

