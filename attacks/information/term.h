/*  $Id$
 *  terminal control characters
 *
 *  Copyright (C) 2006,2007
 *  losTrace aka "David R. Piegdon"
 *  Parity aka "Patrick Sudowe"
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

#ifndef __TERM_H__
#define __TERM_H__


#ifdef USETERM
# define TERM(x,y)	"\x1b[" x ";" y "m"
// font attributes
# define TERM_A_NORMAL	"0"
# define TERM_A_BOLD	"1"
# define TERM_A_ULINE	"2"
# define TERM_A_INVERT	"3"
# define TERM_A_BLINK	"5"
// font colors
//  colors that hit forground
# define TERM_C_FG_BLACK	"30"
# define TERM_C_FG_RED		"31"
# define TERM_C_FG_GREEN	"32"
# define TERM_C_FG_YELLOW	"33"
# define TERM_C_FG_BLUE		"34"
# define TERM_C_FG_MAGENTA	"35"
# define TERM_C_FG_CYAN		"36"
# define TERM_C_FG_WHITE	"37"
//  colors that hit background
# define TERM_C_BG_BLACK	"40"
# define TERM_C_BG_RED		"41"
# define TERM_C_BG_GREEN	"42"
# define TERM_C_BG_YELLOW	"43"
# define TERM_C_BG_BLUE		"44"
# define TERM_C_BG_MAGENTA	"45"
# define TERM_C_BG_CYAN		"46"
# define TERM_C_BG_WHITE	"47"
// some collected attributes
# define TERM_RESET	"\x1b[0m"
# define TERM_FAULT	TERM(TERM_A_BLINK,  TERM_C_BG_RED)
# define TERM_WARNING	TERM(TERM_A_ULINE,  TERM_C_FG_RED)
# define TERM_ATTENT	TERM(TERM_A_NORMAL, TERM_C_FG_BLUE)

# define TERM_RED	TERM(TERM_A_BOLD,   TERM_C_FG_RED)
# define TERM_GREEN	TERM(TERM_A_BOLD,   TERM_C_FG_GREEN)
# define TERM_ORANGE	TERM(TERM_A_NORMAL, TERM_C_FG_YELLOW)
# define TERM_YELLOW	TERM(TERM_A_BOLD,   TERM_C_FG_YELLOW)
# define TERM_BLUE	TERM(TERM_A_BOLD,   TERM_C_FG_BLUE)
# define TERM_MAGENTA	TERM(TERM_A_BOLD,   TERM_C_FG_MAGENTA)
# define TERM_CYAN	TERM(TERM_A_BOLD,   TERM_C_FG_CYAN)
# define TERM_GREY	TERM(TERM_A_BOLD,   TERM_C_FG_BLACK)
# define TERM_WHITE	TERM(TERM_A_BOLD,   TERM_C_FG_WHITE)

#else
# define TERM(x,y)

# define TERM_RESET
# define TERM_FAULT
# define TERM_WARNING
# define TERM_ATTENT

# define TERM_RED
# define TERM_GREEN
# define TERM_ORANGE
# define TERM_YELLOW
# define TERM_BLUE
# define TERM_MAGENTA
# define TERM_CYAN
# define TERM_GREY
# define TERM_WHITE
#endif


#endif // __TERM_H__

