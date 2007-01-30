/*  $Id$
 *  vim: fdm=marker
 *
 *
 *  endian_swapXX will convert from big to little endianess
 *  and also from little to big endianess.
 * 
 *  needed, because htons() et al only do something, if
 *  host-byteoder != network-byteorder. but if we
 *  somehow get data from a different byteordered system
 *  (e.g. via physical mem), we need to change it, even
 *  if htons does not do anything... :(
 *
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

#ifndef __ENDIAN_SWAP_H__
# define __ENDIAN_SWAP_H__

#include <stdint.h>


static inline uint64_t endian_swap64(uint64_t val)
{
	uint64_t sval;

	((uint8_t*)&sval)[0]	= ((uint8_t*)&val)[7];
	((uint8_t*)&sval)[1]	= ((uint8_t*)&val)[6];
	((uint8_t*)&sval)[2]	= ((uint8_t*)&val)[5];
	((uint8_t*)&sval)[3]	= ((uint8_t*)&val)[4];
	((uint8_t*)&sval)[4]	= ((uint8_t*)&val)[3];
	((uint8_t*)&sval)[5]	= ((uint8_t*)&val)[2];
	((uint8_t*)&sval)[6]	= ((uint8_t*)&val)[1];
	((uint8_t*)&sval)[7]	= ((uint8_t*)&val)[0];

	return sval;
}

static inline uint32_t endian_swap32(uint32_t val)
{
	uint32_t sval;

	((uint8_t*)&sval)[0]	= ((uint8_t*)&val)[3];
	((uint8_t*)&sval)[1]	= ((uint8_t*)&val)[2];
	((uint8_t*)&sval)[2]	= ((uint8_t*)&val)[1];
	((uint8_t*)&sval)[3]	= ((uint8_t*)&val)[0];

	return sval;
}

static inline uint16_t endian_swap16(uint32_t val)
{
	uint16_t sval;

	((uint8_t*)&sval)[0]	= ((uint8_t*)&val)[3];
	((uint8_t*)&sval)[1]	= ((uint8_t*)&val)[2];

	return sval;
}

#endif // __ENDIAN_SWAP_H__

