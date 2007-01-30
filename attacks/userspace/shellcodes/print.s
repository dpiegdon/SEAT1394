;
; $Id$
;
; print.s: simple shellcode that just prints a statement and then exits
;
; Copyright (C) 2006,2007
; losTrace aka "David R. Piegdon"
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License version 2, as
; published by the Free Software Foundation.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;

BITS 32

	call		string_get_address
	;                1234567890123456789012345678901
	db		'foobar23 - marked by SEAT1394', 0xd, 0xa, 0
string_get_address:
	xor		eax,eax
	mov		al, 4				; call write
	xor		ebx,ebx
	inc		ebx				; stdout (1) as FD
	pop		ecx				; string address
	xor		edx,edx
	mov		dl,0x20				; size of string
	int		0x80

;	xor		eax,eax
;	mov		al,6				; close stdin
;	xor		ebx,ebx
;	inc		ebx
;	int		0x80

	xor		eax,eax
	mov		al,0x01
	xor		ebx,ebx				; exit code
	int		0x80

	ret
