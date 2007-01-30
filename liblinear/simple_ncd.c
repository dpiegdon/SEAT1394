/*  $Id$
 *  liblinear
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

// libcomplearn (http://complearn.org/) is great, but far too complex for this.
// so here my own minimalistic approach

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <bzlib.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// compute simple NCD between blob x and blob y
float simple_ncd(void* x, size_t len_x, void* y, size_t len_y)
{
	void* xy;
	void* C;
	size_t len_C;
	size_t Cx, Cy, Cxy;

	// concat x and y to xy
	xy = malloc(len_x + len_y);
	memcpy(xy, x, len_x);
	memcpy(((char*)xy)+len_x, y, len_y);

	Cx = Cy = Cxy = len_C = (len_x+len_y) * 1.01 + 1000;
	C = malloc(len_C);

	// get compressed sizes for:
	// x
	BZ2_bzBuffToBuffCompress( C, &Cx,  x,  len_x, 9, 0, 20);
	// y
	BZ2_bzBuffToBuffCompress( C, &Cy,  y,  len_y, 9, 0, 20);
	// xy
	BZ2_bzBuffToBuffCompress( C, &Cxy, xy, len_x+len_y, 9, 0, 20);

	// cleanup
	free(xy);
	free(C);

	// calc NCD and return it.
	return (Cxy - MIN(Cx, Cy)) / (float)MAX(Cx,Cy);
}

