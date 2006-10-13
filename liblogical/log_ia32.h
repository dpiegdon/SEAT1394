
#ifndef __LOG_IA32_H__
# define __LOG_IA32_H__

#include "logical.h"

int log_ia32_init(struct logical_handle_data* h);

int log_ia32_finish(struct logical_handle_data* h);

int log_ia32_is_pagedir_fast(struct logical_handle_data* h, addr_t physical_pageno);

float log_ia32_is_pagedir_probability(struct logical_handle_data* h, void* page);

int log_ia32_logical_to_physical(struct logical_handle_data* h, addr_t log_adr, addr_t* physical_adr);

int log_ia32_set_new_pagedirectory(struct logical_handle_data* h, void* pagedir);

#endif // __LOG_IA32_H__

