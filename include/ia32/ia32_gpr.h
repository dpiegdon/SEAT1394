/*  $Id$
 *  vim: fdm=marker
 *
 *  ia32_gpr: struct-definitions for useful general purpose registers
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

#ifndef __IA32_GPR_H__
# define __IA32_GPR_H__

#ifndef __KERNEL__
# include <endian.h>
#else
# undef __BIG_ENDIAN__
# define __LITTLE_ENDIAN__
#endif

// be advised: though here are some endian-corrections, on BIG-endian you will still have
// to swap all bytes. you may use endian_swap32();

struct eflags {
#ifndef __BIG_ENDIAN__
	unsigned int	CF:1;				// (carry flag)
	unsigned int	reserved4:1;			// set to 1
	unsigned int	PF:1;				// (??)
	unsigned int	reserved3:1;			// set to 0
	unsigned int	AF:1;				// (??)
	unsigned int	reserved2:1;			// set to 0
	unsigned int	ZF:1;				// (zero flag)
	unsigned int	SF:1;				// (??)
	unsigned int	TF:1;				// trap flag
	unsigned int	IF:1;				// interrupt enable flag
	unsigned int	DF:1;				// (direction flag)
	unsigned int	OF:1;				// (overflow flag)
	unsigned int	IOPL:2;				// I/O privilege level
	unsigned int	NT:1;				// nested task flag
	unsigned int	reserved1:1;			// set to 0
	unsigned int	RF:1;				// resume flag
	unsigned int	VM:1;				// virtual-8086 mode
	unsigned int	AC:1;				// alignment check
	unsigned int	VIF:1;				// virtual interrupt flag
	unsigned int	VIP:1;				// virtual interrupt pending
	unsigned int	ID:1;				// identification flag
	unsigned int	reserved0:10;			// set to 0
#else
	unsigned int	reserved0:10;			// set to 0
	unsigned int	ID:1;				// identification flag
	unsigned int	VIP:1;				// virtual interrupt pending
	unsigned int	VIF:1;				// virtual interrupt flag
	unsigned int	AC:1;				// alignment check
	unsigned int	VM:1;				// virtual-8086 mode
	unsigned int	RF:1;				// resume flag
	unsigned int	reserved1:1;			// set to 0
	unsigned int	NT:1;				// nested task flag
	unsigned int	IOPL:2;				// I/O privilege level
	unsigned int	OF:1;				// (overflow flag)
	unsigned int	DF:1;				// (direction flag)
	unsigned int	IF:1;				// interrupt enable flag
	unsigned int	TF:1;				// trap flag
	unsigned int	SF:1;				// (??)
	unsigned int	ZF:1;				// (zero flag)
	unsigned int	reserved2:1;			// set to 0
	unsigned int	AF:1;				// (??)
	unsigned int	reserved3:1;			// set to 0
	unsigned int	PF:1;				// (??)
	unsigned int	reserved4:1;			// set to 1
	unsigned int	CF:1;				// (carry flag)
#endif
} __attribute__((__packed__));

#endif // __IA32_GPR_H__

