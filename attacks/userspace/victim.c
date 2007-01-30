/*  $Id$
 *  vim: fdm=marker
 *
 *  victim: just a sample victim process, used during development
 *  	of the inject-code and dmashell tools
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

#include <stdio.h>
#include <unistd.h>

void sub1()
{
	printf("--  in sub 1\n");

	sleep(1);
}

void sub2()
{
	printf("in sub 2   --\n");

	sleep(1);
}




int main()
{
	while(1) {
		sub1();
		sub2();
	}
}
