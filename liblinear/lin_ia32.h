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

#ifndef __LOG_IA32_H__
# define __LOG_IA32_H__

#include "linear.h"

int lin_ia32_init(struct linear_handle_data* h);

int lin_ia32_finish(struct linear_handle_data* h);

int lin_ia32_is_pagedir_fast(struct linear_handle_data* h, addr_t physical_pageno);

float lin_ia32_is_pagedir_probability(struct linear_handle_data* h, void* page);

int lin_ia32_linear_to_physical(struct linear_handle_data* h, addr_t lin_adr, addr_t* physical_adr);

int lin_ia32_set_new_pagedirectory(struct linear_handle_data* h, void* pagedir);

#endif // __LOG_IA32_H__

