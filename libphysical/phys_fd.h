
#ifndef __PHYS_FD_H__
# define __PHYS_FD_H__

#include <stdint.h>

#include "physical.h"

int physical_fd_init(struct physical_handle_data* h);

int physical_fd_finish(struct physical_handle_data* h);

int physical_fd_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);

int physical_fd_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);

int physical_fd_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

int physical_fd_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

#endif // __PHYS_FD_H__

