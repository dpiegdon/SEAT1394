
#ifndef __IA32_CONTROL_REGISTERS_H__
# define __IA32_CONTROL_REGISTERS_H__

#ifdef __BIG_ENDIAN__
# error ia32_control_registers.h not corrected for endianness, yet
#endif

struct cr0 {
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
} __attribute__((__packed__));

struct cr1 {
	unsigned int	reserved0;			//
} __attribute__((__packed__));

struct cr2 {
	unsigned int	pagefault_adr;			// the linear address that caused a page fault
} __attribute__((__packed__));

struct cr3 {
	unsigned int	reserved1:3;			//
	unsigned int	PWT:1;				// page-level writes transparent
	unsigned int	PCD:1;				// page-level cache disable
	unsigned int	reserved0:7;			//
	unsigned int	pagedir_base:20;		// base of page-directory (4kb-page number. shift left 12 and it becomes the physical adr)
} __attribute__((__packed__));

struct cr4 {
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
} __attribute__((__packed__));

#ifdef IA64
struct cr8 {
	unsigned int	TPL:4;				// task priority level
	unsigned int	reserved0:28;			//
} __attribute__((__packed__));
#endif

#endif // __IA32_CONTROL_REGISTERS_H__

