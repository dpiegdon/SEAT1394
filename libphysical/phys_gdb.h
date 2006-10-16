
#ifndef __PHYS_QEMU_GDB_H__
# define __PHYS_QEMU_GDB_H__

#include <stdint.h>

#include "physical.h"

int physical_gdb_init(struct physical_handle_data* h);

int physical_gdb_finish(struct physical_handle_data* h);

int physical_gdb_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);

int physical_gdb_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);

int physical_gdb_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

int physical_gdb_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

#endif // __PHYS_QEMU_GDB_H__

