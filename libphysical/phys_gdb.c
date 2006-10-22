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

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "phys_gdb.h"

// protocol definition:
// http://www1.jct.ac.il/cc-res/online-doc/gdb/gdb_109.html

////// START GDB PROTOCOL STUFF

#define MAX_MESSAGESIZE 1029
static char gdb_buffer[MAX_MESSAGESIZE];

static int gdb_flush_read(struct physical_handle_data* h)
{
	int l = 0;
	int m = 0;
	
	while(l >= 0) {
		l = read(h->data.gdb.fd, gdb_buffer, 1024);
		if( l > 0) {
			m+=l;
			gdb_buffer[l] = 0;
			printf("gdb_flush_read(): flushed %d bytes: \"%s\"\n", l, gdb_buffer);
		}
		
	}
	return m;
}

static int gdb_send_payload(struct physical_handle_data* h, char*data, size_t len)
{
	int checksum;

	if(len > MAX_MESSAGESIZE-5)
		return -EMSGSIZE;

	gdb_buffer[0] = '$';
	memcpy(gdb_buffer+1, data, len);
	gdb_buffer[len+1] = '#';

	checksum = 0;
	for(; len >= 0; len--)
		checksum += data[len];
	checksum %= 256;

	snprintf(gdb_buffer+len+2, 2, "%02x", checksum);
	gdb_buffer[len+4] = 0;

	// send gdb_buffer, <len+4>
	write(h->data.gdb.fd, gdb_buffer, len+4);
}

static int gdb_receive_payload(struct physical_handle_data* h)
{	
	int l = 0;
	int m = 0;
	
	while(l >= 0) {
		l = read(h->data.gdb.fd, gdb_buffer, 1024);
		if( l > 0) {
			m+=l;
			gdb_buffer[l] = 0;
			printf("gdb_receive_payload(): flushed %d bytes: \"%s\"\n", l, gdb_buffer);
		}
		
	}
	return m;
}

////// END GDB PROTOCOL STUFF



int physical_gdb_init(struct physical_handle_data* h)
{
	// this stub is not ready, yet.
	return -1;
}

int physical_gdb_finish(struct physical_handle_data* h)
{
	return 0;
}

int physical_gdb_read(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	char cmd_buffer[64];
	while(len > 0) {
		gdb_flush_read(h);
		// send gdb-command
		snprintf(cmd_buffer, 64, "m%llx,%x", adr, len);
		gdb_send_payload(h, cmd_buffer, strlen(cmd_buffer));
		gdb_receive_payload(h);

		// read stuff
	}

	return 0;
}

int physical_gdb_write(struct physical_handle_data* h, addr_t adr, void* buf, size_t len)
{
	// TODO

	return 0;
}

int physical_gdb_read_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_gdb_read(h, pagenum * h->pagesize, buf, h->pagesize);
}

int physical_gdb_write_page(struct physical_handle_data* h, addr_t pagenum, void* buf)
{
	return physical_gdb_write(h, pagenum * h->pagesize, buf, h->pagesize);
}

