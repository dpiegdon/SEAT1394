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

#include "phys_gdb.h"

// protocol definition:
// http://www1.jct.ac.il/cc-res/online-doc/gdb/gdb_109.html

int physical_gdb_init(struct physical_handle_data* h)
{
	// this stub is not ready, yet.
	return -1;
}

int physical_gdb_finish(struct physical_handle_data* h)
{
	return 0;
}

int physical_gdb_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	while(len) {
		
		len--;
	}

	return 0;
}

int physical_gdb_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	// TODO

	return 0;
}

int physical_gdb_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_gdb_read(h, pagenum * h->pagesize, buf, h->pagesize);
}

int physical_gdb_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_gdb_write(h, pagenum * h->pagesize, buf, h->pagesize);
}

