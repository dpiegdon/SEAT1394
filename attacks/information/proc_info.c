/*  $Id$
 *  vim: fdm=marker
 *
 *  proc_info : functions to get information about a process
 *                 running in a given linear address space
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

// needed for memmem:
#define _GNU_SOURCE

#include <string.h>

#include <endian_swap.h>
#include <linear.h>

// proc_info reconstructs the argument-vector and environment-vector of a
// process running in the linear address space h.
//
// returns 1 on success, 0 on fail. fails, if:
//    * we found that there should be a stackpage, but it is unreadable
//    * we were unable to retrieve neccessary info from the stack
//      (*bin may be set in this case)
//
// on success:
//	if *bin == NULL, thread has no stack page (probably a kernel-thread)
//	if *bin != NULL, *arg_v and *env_v should be malloc()d buffers
//		containing the vectors.
int proc_info(linear_handle h, int *arg_c, char ***arg_v, int *env_c, char ***env_v, char **bin)
{{{

	// on linux, the max environment and argument is limited by PAGESIZE * MAX_ARG_PAGES
	// MAX_ARG_PAGES is 32 (statically defined in include/linux/binfmts.h)
#	define MAX_ARG_PAGES 32
#	define STRING_SEEK_START(p) { if(*p) {while(*p) p--; p++;} }

	static char stack[4096*MAX_ARG_PAGES];
	addr_t pn;
	addr_t stack_bottom_page;
	int valid_page;

	char *p;
	int i;

	char *rargv0p;
	char *largv0p;
	char **argv;
	int argc;
	char **largv;

	char *envv0p;
	int envc;
	char **lenvv;

	// validate return values
	*bin = NULL;
	*arg_c = *env_c = 0;
	*arg_v = *env_v = NULL;

	// seek stack-bottom
	pn = 0xbffff;
	valid_page = 0;
	while(!valid_page && pn >= 0xbf000) {
		if(linear_to_physical(h, pn*4096, &stack_bottom_page)) {
			// page is not mapped
			pn--;
		} else {
			// found stack bottom
			valid_page = 1;
		}
	}
	if(!valid_page) {
		// there was no stack...
		// may be a kernel-thread. so this MAY be no error
		return 1;
	}
	stack_bottom_page = pn;

	// obtain stack
	if(linear_read_page(h, stack_bottom_page, stack+4096*(MAX_ARG_PAGES-1))) {
		// the stack bottom was not readable...
		*bin = NULL;
		return 0;
	}
	// now read the following pages
	bzero(stack,4096*(MAX_ARG_PAGES-1));
	// page "0" was read above.
	for(i = 1; i < MAX_ARG_PAGES; i++)
		linear_read_page(h, stack_bottom_page - i, stack + 4096 * (MAX_ARG_PAGES-(i+1)));

	// find name of binary
	p = stack + 4096*MAX_ARG_PAGES - 6;
	STRING_SEEK_START(p);
	*bin = p;

	// now walk backwards until we find two consequtive NUL-chars.
	// this marks the beginning of args, env and name of binary.
	// point to the first non-NUL-char, that is argv[0]
	while (*(p-1) || *(p-2))
		p--;
	// this is the local address of argv[0]
	largv0p = p;
	// this is the remote address of argv[0]
	rargv0p = (char*)((uint32_t)(p - stack) + (((uint32_t)stack_bottom_page - MAX_ARG_PAGES + 1) * 4096));
#ifdef __BIG_ENDIAN__
	rargv0p = (char*)endian_swap32((uint32_t)rargv0p);
#endif

	// now find the address of argv[0] in the stack;
	// that would be argv[] then.
	// we need to search the LAST hit on the stack, as the program itself
	// may have pushed a copy of the address onto the stack.
	p = stack; argv = NULL;
	while(p) {
		argv = (char**)p;
		p = memmem(p+1, MAX_ARG_PAGES * 4096 - (p-stack), &rargv0p, 4);
	}
	if(!argv) {
		return 0;
	}
	// argc is stored right before argv[]
	argc = *((uint32_t*)(argv - 1));
#ifdef __BIG_ENDIAN__
	argc = endian_swap32(argc);
#endif
	if(argc > 32768) {
		// oops... linux does not allow more than 32768 args...
		return 0;
	}
	if(argc < 1) {
		// oops... a program never can have <1 argument,
		// as argv[1] is the string by which it was execve()'d
		return 0;
	}

	// now that we do have argc, we know how many arguments are real arguments, even if they contain an '=' (like "--foo=bar"). we could
	// use the argv and envv, but some application modify or delete these. so we will parse the list and create our own vectors.

	// create largv
	largv = malloc(sizeof(char*) * (argc+1));
	largv[0] = largv0p;
	largv[argc] = NULL;
	p = largv0p;
	for(i = 1; i < argc; i++) {
		// seek end of string
		while(*p && (p < ((*bin) - 1)))
			p++;
		// go to next
		p++;
		if(p >= *bin-1) {
			// argc was too big?!
			free(largv);
			return 0;
		}
		largv[i] = p;
	}
	p = p + strlen(p) + 1;
	envv0p = p;

	// count env-vars
	envc = 0;
	while(p < (*bin - 1)) {
		envc++;
		while(*p)
			p++;
		p++;
	}
	// and create envv
	lenvv = malloc(sizeof(char*) * (envc+1));
	lenvv[0] = envv0p;
	lenvv[envc] = NULL;
	p = envv0p;
	for(i = 1; i < envc; i++) {
		// seek end of string
		while(*p && (p < (*bin - 1)))
			p++;
		// go to next
		p++;
		if(p < *bin)
			lenvv[i] = p;
	}

	*arg_c = argc;
	*arg_v = largv;

	*env_c = envc;
	*env_v = lenvv;

	return 1;
}}}


// resolve "resolve" from an environment vector.
// returns a pointer INTO envv
char *resolve_env(char **envv, char*resolve)
{{{
	int n;
	int len;

	n = 0;
	len = strlen(resolve);
	while(envv[n]) {
		if(0 == strncmp(envv[n], resolve, len) ) {
			if('=' == envv[n][len]) {
				return &(envv[n][len+1]);
			}
		}
		n++;
	}
	return NULL;
}}}

