
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

