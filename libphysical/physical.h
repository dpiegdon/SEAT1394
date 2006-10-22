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

#ifndef __PHYSICAL_H__
# define __PHYSICAL_H__

#include <stdint.h>
#include <stdlib.h>
#include <libraw1394/raw1394.h>

// addressing is done via 64-bit unsigned physical addresses
typedef uint64_t addr_t;

// when associating a handle, this describes the type of memory-source
// "none" is only for internal purposes. using "none" when associating
// will return a fault
enum physical_type {
	physical_none		= 0,	// internal only
	physical_filedescriptor,	// access phys mem via a given FD. all files should be opened with O_LARGEFILE to allow 64bit-addressing!
	physical_ieee1394,		// access phys mem via ieee1394 (firewire), using libraw1394
	physical_gdb			// access phys mem via the gdb serial protocol
					// typically this will not be useful, only in combination with a VM like qemu that supports gdb
// TODO physical_qemu_savedvm		// access phys mem in a saved vm-status-file of qemu
// TODO physical_suspended		// access phys mem in a suspend-to-disk-image
};

// when associating a handle with a specific type of phys access,
// special parameters are needed to use the to the phys memory device
struct physical_type_data_none {
	int foo;			// unused
};
struct physical_type_data_filedescriptor {
	int fd;				// FD to use
};
struct physical_type_data_ieee1394 {
	raw1394handle_t	raw1394handle;	// raw1394 handle (handle must have a port set
					// via raw1394_set_port().)
	nodeid_t	raw1394target;	// target on the firewire bus
};
struct physical_type_data_gdb {
	int fd;				// FD of connected gdb (e.g. open a tcp-socket to a gdb)
					// has to be opened O_NONBLOCK !
};
union physical_type_data {
	struct physical_type_data_none			none;
	struct physical_type_data_filedescriptor	filedescriptor;
	struct physical_type_data_ieee1394		ieee1394;
	struct physical_type_data_gdb			gdb;
};


// internally, all needed data is stored here. physical_handle is really a pointer to this.
struct physical_handle_data {
	enum	physical_type		type;				// type of memory source
	union	physical_type_data	data;				// data specific to type
	size_t				pagesize;			// typical pagesize in this source

	// type-specific functions:
	int(*finish)(struct physical_handle_data* h);

	int(*read)(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);
	int(*write)(struct physical_handle_data* h, addr_t adr, void* buf, size_t len);
	int(*read_page)(struct physical_handle_data* h, addr_t pagenum, void* buf);
	int(*write_page)(struct physical_handle_data* h, addr_t pagenum, void* buf);
};

// one can access multiple physical memory sources. to do this, one
// requests a handle and associates the handle with the source of
// choice.
typedef struct physical_handle_data* physical_handle;

///////////// functions

// request a new handle. returns NULL on error
physical_handle physical_new_handle();

// associate a given handle with a physical memory source of type <type>, typical pagesize <pagesize>
int physical_handle_associate(physical_handle h, enum physical_type type, union physical_type_data *data, size_t pagesize);

// release a handle (and remove it)
int physical_handle_release(physical_handle h);


// read physical memory: read at address <adr>, <len> bytes and
// store them into <buf>
int physical_read(physical_handle h, addr_t adr, void* buf, size_t len);

// write physical memory: write at address <adr>, len bytes from <buf>
int physical_write(physical_handle h, addr_t adr, void* buf, size_t len);

// read the full physical page no. <pagenum> into <buf>
int physical_read_page(physical_handle h, addr_t pagenum, void* buf);

// write the full page in <buf> to the physical page no. <pagenum>
int physical_write_page(physical_handle h, addr_t pagenum, void* buf);

#endif // __PHYSICAL_H__

