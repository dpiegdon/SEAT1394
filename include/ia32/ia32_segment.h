/*  $Id$
 *  vim: fdm=marker
 *
 *  ia32_segment: struct-definitions for segmentation structures
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

#ifndef __IA32_SEGMENT_H__
# define __IA32_SEGMENT_H__

#ifndef __KERNEL__
# include <endian.h>
# include <stdint.h>
#else
# undef __BIG_ENDIAN__
# define __LITTLE_ENDIAN__
#endif

// be advised: though here are some endian-corrections, on BIG-endian you will still have
// to swap all bytes. you may use endian_swap32();

struct segment_selector {
#ifndef __BIG_ENDIAN__
	unsigned int	RPL:2;				// requested privilege level
	unsigned int	TI:1;				// table indicator: 0 = GDT, 1 = LDT
	unsigned int	index:13;			// table index
#else
	unsigned int	index:13;			// table index
	unsigned int	TI:1;				// table indicator: 0 = GDT, 1 = LDT
	unsigned int	RPL:2;				// requested privilege level
#endif
} __attribute__((__packed__));

union segment {
	uint16_t				raw;
	struct segment_selector			selector;
} __attribute__((__packed__));

struct segment_descriptor_raw {
#ifndef __BIG_ENDIAN__
	unsigned int	low:32;				//
	unsigned int	high:32;			//
#else
	unsigned int	high:32;			//
	unsigned int	low:32;				//
#endif
} __attribute__((__packed__));

struct segment_descriptor_unpresent {
#ifndef __BIG_ENDIAN__
	// high 32 bit:
	unsigned int	available0:16;			//
	unsigned int	reserved0:1;			// set to 0
	unsigned int	DPL:2;				// descriptor privilege level
	unsigned int	S:1;				// segment descriptor type (system-segment: 0, code or data segment: 1)
	unsigned int	type:4;				// segment or gate type, access type
	unsigned int	available1:8;			//
	// low 32 bit:
	unsigned int	available2:32;			//
#else
	// low 32 bit:
	unsigned int	available2:32;			//
	// high 32 bit:
	unsigned int	available1:8;			//
	unsigned int	type:4;				// segment or gate type, access type
	unsigned int	S:1;				// segment descriptor type (system-segment: 0, code or data segment: 1)
	unsigned int	DPL:2;				// descriptor privilege level
	unsigned int	reserved0:1;			// set to 0
	unsigned int	available0:16;			//
#endif
} __attribute__((__packed__));

struct segment_descriptor_present {
#ifndef __BIG_ENDIAN__
	// low 32 bit:
	unsigned int	seglimit_low:16;		// segment limit, bits 15-0
	unsigned int	base_low:16;			// base address, bits 15-0
	// high 32 bit:
	unsigned int	base_med:8;			// base address, bits 23-16
	unsigned int	type:4;				// segment or gate type, access type
	unsigned int	S:1;				// segment descriptor type (system-segment: 0, code or data segment: 1)
	unsigned int	DPL:2;				// descriptor privilege level
	unsigned int	P:1;				// segment present (1: present in physical mem)
	unsigned int	seglimit_high:4;		// segment limit, bits 19-16
	unsigned int	AVL:1;				// available for system programmer
# ifdef IA64
	unsigned int	L:1;				// 64-bit code segment
# else
	unsigned int	reserved0:1;			// reserved, set to 0
# endif
	unsigned int	D_B:1;				// default operation size, default SP size and/or upper bound
	unsigned int	G:1;				// granularity (0: segment-limit in bytes, 1: segment-limit in 4KB-units)
	unsigned int	base_high:8;			// base address, bits 31-24
#else
	// high 32 bit:
	unsigned int	base_high:8;			// base address, bits 31-24
	unsigned int	G:1;				// granularity (0: segment-limit in bytes, 1: segment-limit in 4KB-units)
	unsigned int	D_B:1;				// default operation size, default SP size and/or upper bound
# ifdef IA64
	unsigned int	L:1;				// 64-bit code segment
# else
	unsigned int	reserved0:1;			// reserved, set to 0
# endif
	unsigned int	AVL:1;				// available for system programmer
	unsigned int	seglimit_high:4;		// segment limit, bits 19-16
	unsigned int	P:1;				// segment present (1: present in physical mem)
	unsigned int	DPL:2;				// descriptor privilege level
	unsigned int	S:1;				// segment descriptor type (system-segment: 0, code or data segment: 1)
	unsigned int	type:4;				// segment or gate type, access type
	unsigned int	base_med:8;			// base address, bits 23-16
	// low 32 bit:
	unsigned int	base_low:16;			// base address, bits 15-0
	unsigned int	seglimit_low:16;		// segment limit, bits 15-0
#endif
} __attribute__((__packed__));

union segment_descriptor {
	struct segment_descriptor_raw		raw;
	struct segment_descriptor_unpresent	unpresent;
	struct segment_descriptor_present	present;
} __attribute__((__packed__));

// TODO: enum for segment-type

#endif // __IA32_SEGMENT_H__

