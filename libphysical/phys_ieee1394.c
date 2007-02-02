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

/*
 * due to random bus resets the following will be implemented:
 *
 * init():
 * 	if data->guid = 0
 * 		obtain guid from nid
 *
 * install reset-handler, that gets nid from guid after bus-reset
 */

// FIXME: remove debugging stuff and stdio.h
#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <endian.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include "phys_ieee1394.h"
#include "endian_swap.h"

#define MIN(a,b)  ( ((a) < (b)) ? (a) : (b) )

#define RAWHANDLE (h->data.ieee1394.raw1394handle)
#define TARGET    (h->data.ieee1394.raw1394target_nid)

#define NODE_OFFSET 0xffc0

// blocksize is auto-adjusted down to 4 in case of errors
#define BLOCKSIZE_MAX_VAL 1024
#define BLOCKSIZE_MIN_VAL 4
static size_t blocksize = BLOCKSIZE_MAX_VAL;
static bus_reset_handler_t old_reset_handler = NULL;

// get a targets GUID from its nodeid
// on error, returns 0 as GUID
static uint64_t guid_from_nodeid(raw1394handle_t handle, nodeid_t node)
{{{
	uint64_t guid = 0;
	uint32_t low = 0;
	uint32_t high = 0;
	raw1394_read(handle, node, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x0c, 4, &low);
	raw1394_read(handle, node, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x10, 4, &high);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	guid = (((uint64_t)high) << 32) | low;
	guid = endian_swap64(guid);
#else
	guid = (((uint64_t)low) << 32) | high;
#endif

	return guid;
}}}

// returns 0 if GUID found and stores nodeid in nodeid
// returns !0 on error
static int nodeid_from_guid(raw1394handle_t handle, uint64_t guid, nodeid_t *nodeid)
{{{
	int nodecount;
	int i;
	uint64_t it_guid;

	nodecount = raw1394_get_nodecount(handle);

	for(i = NODE_OFFSET; i < (NODE_OFFSET + nodecount); i++) {
		it_guid = guid_from_nodeid(handle, i);
		if(guid == it_guid) {
			*nodeid = i;
			return 0;
		}
	}

	return 1;
}}}

static int reset_iterator(physical_handle h, void *data)
{{{
	nodeid_t nid;

	// (*data) is a raw1394handle_t
	// check if this is a matching handle
	if(h->data.ieee1394.raw1394handle != *(raw1394handle_t*)data)
		return 0;

	// search the GUID on the bus, save its nodeid. if not found,
	// initiate a RAW1394_LONG_RESET and do the same.
	while(nodeid_from_guid(RAWHANDLE, h->data.ieee1394.raw1394target_guid, (void*)&nid)) {
		fprintf(stderr,"could not find node %016llX. please check ieee1394 connection.\n", h->data.ieee1394.raw1394target_guid);
		//raw1394_reset_bus_new(RAWHANDLE, RAW1394_LONG_RESET);
		sleep(2);
	}
	fprintf(stderr, "new nid: 0x%04x ", nid);
	TARGET = nid;

	// FIXME:
	// if then the host can not be found, print an error message
	// and just continue as if nothing happened (let the user manually reset
	// the bus, hopefully)
	
	return 0;
}}}

static int reset_handler(raw1394handle_t handle, unsigned int generation)
{{{
	int ret = 0; // ??

	fprintf(stderr, "\n(physical_ieee1394) bus reset, "); // let iterator finish the line.

	// call former handler
	if(old_reset_handler)
		ret = old_reset_handler(handle, generation);

	// iterate over all physical handles with type==physical_ieee1394
	physical_iterate_all_handles(physical_ieee1394, reset_iterator, &handle);

	fprintf(stderr, "\n");

	return ret;
}}}

int physical_ieee1394_init(struct physical_handle_data* h)
{
	// developer shall give us a handle that is already
	// associated with a firewire port

	// if GUID == 0, get GUID from NID
	if(h->data.ieee1394.raw1394target_guid == 0) {
		h->data.ieee1394.raw1394target_guid = guid_from_nodeid(RAWHANDLE, TARGET);
		if(h->data.ieee1394.raw1394target_guid == 0) {
			fprintf(stderr, "phys1394: failed to find node 0x%04x on the given bus\n", h->data.ieee1394.raw1394target_nid);
			return -1;
		}
//		fprintf(stderr, "phys1394: got guid %016llX for nodeid 0x%04x\n", h->data.ieee1394.raw1394target_guid, TARGET);
	} else {
		// otherwise, get NID from GUID
		if(nodeid_from_guid(RAWHANDLE, h->data.ieee1394.raw1394target_guid, &(TARGET))) {
			// failed to find GUID on this bus
			fprintf(stderr, "phys1394: failed to find %016llx on the given bus\n", h->data.ieee1394.raw1394target_guid);
			return -1;
		}
	}

	// install our bus-reset-handler that will search the guid
	// after a bus-reset
	old_reset_handler = raw1394_set_bus_reset_handler(RAWHANDLE, reset_handler);
	//raw1394_busreset_notify(RAWHANDLE, RAW1394_NOTIFY_ON);

	return 0;
}

int physical_ieee1394_finish(struct physical_handle_data* h)
{
	return 0;
}

int physical_ieee1394_read(struct physical_handle_data* h, addr_t adr, void* buf, unsigned long int len)
{
	int err;
	size_t r = 0;		// how much was read so far
	size_t tr;		// how much is to read

	if(blocksize < BLOCKSIZE_MAX_VAL)
		blocksize = blocksize << 2;

	while(len > 0) {
		tr = MIN(len, blocksize);
		// argh pointer arithmetic... careful with this quadlet_t ...
		err = raw1394_read(RAWHANDLE, TARGET, adr + r, tr, (quadlet_t*)((char*)buf + r));
		if(err < 0) {
			// auto-adjust blocksize to specific machine
			if(blocksize > BLOCKSIZE_MIN_VAL) {
				blocksize = blocksize >> 2;
//				printf("adjusted blocksize to %d\n", blocksize);
			} else {
				return err;
			}
		} else {
			len -= tr;
			r += tr;
		}
	}

	return 0;
}

int physical_ieee1394_write(struct physical_handle_data* h, addr_t adr, void* buf, unsigned long int len)
{
	int err;
	size_t w = 0;		// how much was written so far
	size_t tw;		// how much is to write

	if(blocksize < BLOCKSIZE_MAX_VAL)
		blocksize = blocksize << 2;

	while(len > 0) {
		tw = MIN(len, blocksize);
		/// argh pointer arithmetic... careful with this quadlet_t ...
		err = raw1394_write(RAWHANDLE, TARGET, adr + w, tw, (quadlet_t*)((char*)buf + w));
		if(err < 0) {
			// auto-adjust blocksize to specific machine
			if(blocksize > BLOCKSIZE_MIN_VAL) {
				blocksize = blocksize >> 2;
//				printf("adjusted blocksize to %d\n", blocksize);
			} else {
				return err;
			}
		} else {
			len -= tw;
			w += tw;
		}
	}

	return 0;
}

int physical_ieee1394_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_ieee1394_read(h, pagenum * h->pagesize, buf, h->pagesize);
}

int physical_ieee1394_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_ieee1394_write(h, pagenum * h->pagesize, buf, h->pagesize);
}

