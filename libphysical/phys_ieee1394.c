/*  $Id$
 *  libphysical
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
#include <stdlib.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include "phys_ieee1394.h"

#define MIN(a,b)  ( ((a) < (b)) ? (a) : (b) )

#define RAWHANDLE (h->data.ieee1394.raw1394handle)
#define TARGET    (h->data.ieee1394.raw1394target)
//#define BLOCKSIZE 1024
#define BLOCKSIZE 4

int physical_ieee1394_init(struct physical_handle_data* h)
{
	// developer shall give us a handle that is already associated with a target on the firewire bus
	// then we won't need to do anything else.
	return 0;
}

int physical_ieee1394_finish(struct physical_handle_data* h)
{
	return 0;
}

int physical_ieee1394_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	int err;
	size_t r = 0;		// how much was read so far
	size_t tr;		// how much is to read

	while(len > 0) {
		tr = MIN(len, BLOCKSIZE);
		/// argh pointer arithmetic... careful with this quadlet_t ...
		err = raw1394_read(RAWHANDLE, TARGET, adr + r, tr, (quadlet_t*)((char*)buf + r));
		if(err)
			return err;
		len -= tr;
		r += tr;
	}

	return 0;
}

int physical_ieee1394_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	int err;
	size_t w = 0;		// how much was written so far
	size_t tw;		// how much is to write

	while(len > 0) {
		tw = MIN(len, BLOCKSIZE);
		/// argh pointer arithmetic... careful with this quadlet_t ...
		err = raw1394_write(RAWHANDLE, TARGET, adr + w, tw, (quadlet_t*)((char*)buf + w));
		if(err)
			return err;
		len -= tw;
		w += tw;
	}

	return 0;
}

int physical_ieee1394_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_ieee1394_read(h, pagenum * h->pagesize, buf, h->pagesize);
}

int physical_ieee1394_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_ieee1394_write(h, pagenum * h->pagesize, buf, h->pagesize);
}

