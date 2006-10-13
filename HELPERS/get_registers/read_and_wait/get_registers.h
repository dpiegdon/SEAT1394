
#ifndef GET_REGISTERS_H
# define GET_REGISTERS_H

#include <ia32/ia32_control_registers.h>
#include <ia32/ia32_segment.h>

struct register_dump {
	uint32_t cr0;
	uint32_t cr3;
	uint32_t cr4;

	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	uint16_t cs;
	uint16_t ds;
	uint16_t es;
	uint16_t fs;
	uint16_t gs;
	uint16_t ss;
} __attribute__ ((__packed__));

struct register_parsed {
	struct cr0 cr0;
	struct cr3 cr3;
	struct cr4 cr4;

	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	struct segment_selector cs;
	struct segment_selector ds;
	struct segment_selector es;
	struct segment_selector fs;
	struct segment_selector gs;
	struct segment_selector ss;
} __attribute__ ((__packed__));

union registers {
	struct register_dump dump;
	struct register_parsed parsed;
} __attribute__ ((__packed__));

#endif // GET_REGISTERS_H

