/*  $Id$
 *  vim: fdm=marker
 *
 *  proc_info : functions to get information about a process
 *                 running in a given linear address space
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
int proc_info(linear_handle h, int *arg_c, char ***arg_v, int *env_c, char ***env_v, char **bin);


// resolve "resolve" from an environment vector.
// returns a pointer INTO envv
char *resolve_env(char **envv, char*resolve);


