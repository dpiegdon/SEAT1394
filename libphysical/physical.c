/*  $Id$
 *  libphysical
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

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "physical.h"
#include "phys_fd.h"
#include "phys_ieee1394.h"
#include "phys_gdb.h"

// request a new handle. returns NULL on error
physical_handle __attribute__ ((visibility ("default")))
physical_new_handle()
{
	physical_handle h;

	h = malloc(sizeof(struct physical_handle_data));
	if(h) {
		h->type = physical_none;
	}

	return h;
}

// associate a given handle with a physical memory source of type <type>, typical pagesize <pagesize>
int __attribute__ ((visibility ("default")))
physical_handle_associate(physical_handle h, enum physical_type type, union physical_type_data *data, size_t pagesize)
{
	if(!h)
		return -EBADR;

	// can not access source 'physical_none'
	if(type == physical_none)
		return -EBADRQC;	// bad request code

	// handle already used for a source.
	if(h->type != physical_none)
		return -EINVAL;		// invalid argument

	h->data = *data;
	h->pagesize = pagesize;
	switch(type) {
		case physical_none:
			// will never happen. only prevents compiler-warning.
			break;
		default:
			return -EBADRQC;
			break;
		case physical_filedescriptor:
			h->type		= physical_filedescriptor;

			h->finish	= physical_fd_finish;
			h->read		= physical_fd_read;
			h->write	= physical_fd_write;
			h->read_page	= physical_fd_read_page;
			h->write_page	= physical_fd_write_page;

			if(physical_fd_init(h)) {
				h->type = physical_none;
				return -EINVAL;
			}
			break;
		case physical_ieee1394:
			h->type		= physical_ieee1394;

			h->finish	= physical_ieee1394_finish;
			h->read		= physical_ieee1394_read;
			h->write	= physical_ieee1394_write;
			h->read_page	= physical_ieee1394_read_page;
			h->write_page	= physical_ieee1394_write_page;

			if(physical_ieee1394_init(h)) {
				h->type = physical_none;
				return -EINVAL;
			}
			break;
		case physical_gdb:
			h->type		= physical_gdb;

			h->finish	= physical_gdb_finish;
			h->read		= physical_gdb_read;
			h->write	= physical_gdb_write;
			h->read_page	= physical_gdb_read_page;
			h->write_page	= physical_gdb_write_page;

			if(physical_gdb_init(h)) {
				h->type = physical_gdb;
				return -EINVAL;
			}
			break;
	}

	return 0;
}

// release a handle (and remove it)
int __attribute__ ((visibility ("default")))
physical_handle_release(physical_handle h)
{
	if(!h)
		return -EBADR;		// bad request descriptor

	// finish source specific stuff
	h->finish(h);

	free(h);
	return 0;
}

// read physical memory: read at address <adr>, <len> bytes and
// store them into <buf>
int __attribute__ ((visibility ("default")))
physical_read(physical_handle h, addr_t adr, void* buf, unsigned long int len)
{
	if(h && (h->type != physical_none)) {
		return h->read(h,adr,buf,len);
	} else
		return -EBADR;			// bad request descriptor
}

// write physical memory: write at address <adr>, len bytes from <buf>
int __attribute__ ((visibility ("default")))
physical_write(physical_handle h, addr_t adr, void* buf, unsigned long int len)
{
	if(h && (h->type != physical_none)) {
		return h->write(h,adr,buf,len);
	} else
		return -EBADR;			// bad request descriptor
}

// read the full physical page no. <pagenum> into <buf>
int __attribute__ ((visibility ("default")))
physical_read_page(physical_handle h, addr_t pagenum, void* buf)
{
	if(h && (h->type != physical_none)) {
		return h->read_page(h,pagenum,buf);
	} else
		return -EBADR;			// bad request descriptor
}

// write the full page in <buf> to the physical page no. <pagenum>
int __attribute__ ((visibility ("default")))
physical_write_page(physical_handle h, addr_t pagenum, void* buf)
{
	if(h && (h->type != physical_none)) {
		return h->write_page(h,pagenum,buf);
	} else
		return -EBADR;			// bad request descriptor
}

