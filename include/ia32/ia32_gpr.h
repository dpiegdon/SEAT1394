
#ifndef __IA32_GPR_H__
# define __IA32_GPR_H__

#ifndef __BIG_ENDIAN__
# error ia32_gpr.h not corrected for endianness, yet
#endif

struct eflags {
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
} __attribute__((__packed__));

#endif // __IA32_GPR_H__

