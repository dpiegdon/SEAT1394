
#ifndef __PHYS_IEEE1394_H__
# define __PHYS_IEEE1394_H__

#include <stdint.h>

#include "physical.h"

int physical_ieee1394_init(struct physical_handle_data* h);

int physical_ieee1394_finish(struct physical_handle_data* h);

int physical_ieee1394_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);

int physical_ieee1394_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);

int physical_ieee1394_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

int physical_ieee1394_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf);

#endif // __PHYS_IEEE1394_H__

