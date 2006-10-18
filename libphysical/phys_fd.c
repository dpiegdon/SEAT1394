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

#define _LARGEFILE64_SOURCE

#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>

#include "phys_fd.h"

int physical_fd_init(struct physical_handle_data* h)
{
	return 0;
}

int physical_fd_finish(struct physical_handle_data* h)
{
	return 0;
}

int physical_fd_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	if( ((off64_t)-1)  == lseek64(h->data.filedescriptor.fd, adr, SEEK_SET))
		return -1;
	if(len != read(h->data.filedescriptor.fd, buf, len))
		return -1;
	return 0;
}

int physical_fd_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	if( ((off64_t)-1)  == lseek64(h->data.filedescriptor.fd, adr, SEEK_SET)) {
		return -1;
	}
	if(len != write(h->data.filedescriptor.fd, buf, len)) {
		return -1;
	}

	return 0;
}

int physical_fd_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	if( ((off64_t)-1)  == lseek64(h->data.filedescriptor.fd, pagenum * h->pagesize, SEEK_SET))
		return -1;
	if(h->pagesize != read(h->data.filedescriptor.fd, buf, h->pagesize))
		return -1;
	return 0;
}

int physical_fd_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	if( ((off64_t)-1)  == lseek64(h->data.filedescriptor.fd, pagenum * h->pagesize, SEEK_SET))
		return -1;
	if(h->pagesize != write(h->data.filedescriptor.fd, buf, h->pagesize))
		return -1;

	return 0;
}

