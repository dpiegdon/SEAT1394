
#ifndef __IA32_CONTROL_REGISTERS_H__
# define __IA32_CONTROL_REGISTERS_H__

#ifndef __KERNEL__
# include <endian.h>
#else
# undef __BIG_ENDIAN__
# define __LITTLE_ENDIAN__
#endif

// be advised: though here are some endian-corrections, on BIG-endian you will still have
// to swap all bytes. you may use endian_swap32();

struct cr0 {
#ifndef __BIG_ENDIAN__
	unsigned int	PE:1;				// protection enable
	unsigned int	MP:1;				// monitor coprocessor (FPU)
	unsigned int	EM:1;				// emulation (FPU)
	unsigned int	TS:1;				// task switched
	unsigned int	ET:1;				// extension type (FPU)
	unsigned int	NE:1;				// numeric error (FPU)
	unsigned int	reserved2:10;			//
	unsigned int	WP:1;				// write protect
	unsigned int	reserved1:1;			//
	unsigned int	AM:1;				// alignment-mask
	unsigned int	reserved0:10;			//
	unsigned int	NW:1;				// not write-through
	unsigned int	CD:1;				// cache disable
	unsigned int	PG:1;				// paging
#else
	unsigned int	PG:1;				// paging
	unsigned int	CD:1;				// cache disable
	unsigned int	NW:1;				// not write-through
	unsigned int	reserved0:10;			//
	unsigned int	AM:1;				// alignment-mask
	unsigned int	reserved1:1;			//
	unsigned int	WP:1;				// write protect
	unsigned int	reserved2:10;			//
	unsigned int	NE:1;				// numeric error (FPU)
	unsigned int	ET:1;				// extension type (FPU)
	unsigned int	TS:1;				// task switched
	unsigned int	EM:1;				// emulation (FPU)
	unsigned int	MP:1;				// monitor coprocessor (FPU)
	unsigned int	PE:1;				// protection enable
#endif
} __attribute__((__packed__));

struct cr1 {
#ifndef __BIG_ENDIAN__
	unsigned int	reserved0;			//
#else
	unsigned int	reserved0;			//
#endif
} __attribute__((__packed__));

struct cr2 {
#ifndef __BIG_ENDIAN__
	unsigned int	pagefault_adr;			// the linear address that caused a page fault
#else
	unsigned int	pagefault_adr;			// the linear address that caused a page fault
#endif
} __attribute__((__packed__));

struct cr3 {
#ifndef __BIG_ENDIAN__
	unsigned int	reserved1:3;			//
	unsigned int	PWT:1;				// page-level writes transparent
	unsigned int	PCD:1;				// page-level cache disable
	unsigned int	reserved0:7;			//
	unsigned int	pagedir_base:20;		// base of page-directory (4kb-page number. shift left 12 and it becomes the physical adr)
#else
	unsigned int	pagedir_base:20;		// base of page-directory (4kb-page number. shift left 12 and it becomes the physical adr)
	unsigned int	reserved0:7;			//
	unsigned int	PCD:1;				// page-level cache disable
	unsigned int	PWT:1;				// page-level writes transparent
	unsigned int	reserved1:3;			//
#endif
} __attribute__((__packed__));

struct cr4 {
#ifndef __BIG_ENDIAN__
	unsigned int	VME:1;				// virtual-8086 mode extension
	unsigned int	PVI:1;				// protected-mode virtual interrupts
	unsigned int	TSD:1;				// time stamp disable
	unsigned int	DE:1;				// debugging extension
	unsigned int	PSE:1;				// page size extension
	unsigned int	PAE:1;				// physical address extension
	unsigned int	MCE:1;				// machine-check enable
	unsigned int	PGE:1;				// page global enable (don't flush pages marked as global in PD or PT)
	unsigned int	PCE:1;				// performance-monitoring counter enable
	unsigned int	OSFXSR:1;			// operating support for FXSAVE and FXRSTOR (FPU)
	unsigned int	OSXMMEXCPT:1;			// operating support for unmasked SIMD floating-point exceptions (FPU)
	unsigned int	reserved0:21;			//
#else
	unsigned int	reserved0:21;			//
	unsigned int	OSXMMEXCPT:1;			// operating support for unmasked SIMD floating-point exceptions (FPU)
	unsigned int	OSFXSR:1;			// operating support for FXSAVE and FXRSTOR (FPU)
	unsigned int	PCE:1;				// performance-monitoring counter enable
	unsigned int	PGE:1;				// page global enable (don't flush pages marked as global in PD or PT)
	unsigned int	MCE:1;				// machine-check enable
	unsigned int	PAE:1;				// physical address extension
	unsigned int	PSE:1;				// page size extension
	unsigned int	DE:1;				// debugging extension
	unsigned int	TSD:1;				// time stamp disable
	unsigned int	PVI:1;				// protected-mode virtual interrupts
	unsigned int	VME:1;				// virtual-8086 mode extension
#endif
} __attribute__((__packed__));

#ifdef IA64
struct cr8 {
#ifndef __BIG_ENDIAN__
	unsigned int	TPL:4;				// task priority level
	unsigned int	reserved0:28;			//
#else
	unsigned int	reserved0:28;			//
	unsigned int	TPL:4;				// task priority level
#endif
} __attribute__((__packed__));
#endif

#endif // __IA32_CONTROL_REGISTERS_H__

