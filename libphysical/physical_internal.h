/*  $Id$
 *  vim: fdm=marker
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

#ifndef __PHYSICAL_INTERNAL_H__
# define __PHYSICAL_INTERNAL_H__

# include "physical.h"

// from physical.c:

typedef int (*physical_iterator_t)(physical_handle h , void*data);

struct handle_list_entry {
	struct handle_list_entry        *prev;
	struct handle_list_entry        *next;

	physical_handle                 h;
};

// call iterator for all handles that are of <type>,
// if type==physical_none, call iterater for all
void physical_iterate_all_handles(enum physical_type type, physical_iterator_t iterator, void* data);

#endif // __PHYSICAL_INTERNAL_H__

