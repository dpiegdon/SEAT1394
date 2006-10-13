/*
 * get_registers: dump all important cpu registers
 */
#include <linux/module.h>
#include <linux/proc_fs.h>

#include "get_registers.h"


void get_registers_getdata(char* dst)
{
	uint32_t v;

	__asm__("mov %%cr0, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).cr0 = v;

	__asm__("mov %%cr3, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).cr3 = v;

	__asm__("mov %%cr4, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).cr4 = v;

	__asm__("mov %%esp, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).esp = v;

	__asm__("mov %%ebp, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).ebp = v;

	__asm__("mov %%esi, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).esi = v;

	__asm__("mov %%edi, %%eax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).edi = v;

	__asm__("mov %%cs, %%ax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).cs = v;

	__asm__("mov %%ds, %%ax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).ds = v;

	__asm__("mov %%es, %%ax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).es = v;

	__asm__("mov %%fs, %%ax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).fs = v;

	__asm__("mov %%gs, %%ax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).gs = v;

	__asm__("mov %%ss, %%ax\n"
		: "=a" (v));
	(*(struct register_dump*)dst).ss = v;
}

int get_registers_proc_read(char* page, char** start, off_t offset, int count, int* eof, void* data)
{
	if(count < sizeof(struct register_dump)) {
		return 0;
	} else {
		*eof = 1;
		get_registers_getdata(page);
		return sizeof(struct register_dump);
	}
}

static struct proc_dir_entry* procfile_registerdump;

static int get_registers_init(void)
{
	printk(KERN_ALERT "get_registers_init()\n");

	procfile_registerdump = create_proc_read_entry("registerdump",
			0400, NULL, get_registers_proc_read, NULL);

	printk(KERN_ALERT "get_registers_init(): done\n");
	return 0;
}

static void get_registers_exit(void)
{
	printk(KERN_ALERT "get_registers_exit()\n");
	remove_proc_entry("registerdump", NULL);
	printk(KERN_ALERT "get_registers_exit(): done\n");
}

MODULE_LICENSE("GPL");
module_init(get_registers_init);
module_exit(get_registers_exit);

