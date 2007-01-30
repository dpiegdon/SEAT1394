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

#ifndef __PHYS_IEEE1394_H__
# define __PHYS_IEEE1394_H__

#include <stdint.h>

#include "physical.h"

int physical_ieee1394_init(struct physical_handle_data* h);

int physical_ieee1394_finish(struct physical_handle_data* h);

int physical_ieee1394_read(struct physical_handle_data* h, addr_t adr, void* buf, unsigned long int len);

int physical_ieee1394_write(struct physical_handle_data* h, addr_t adr, void* buf, unsigned long int len);

int physical_ieee1394_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

int physical_ieee1394_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

#endif // __PHYS_IEEE1394_H__

