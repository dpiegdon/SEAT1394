;
; $Id$
;
; marker.s: a shellcode that is meant to be prefixed to other shellcodes.
;	during execution it will possibly load a new stackpointer and
;	NOT the "marker", so an attacker cann see that it was executed.
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

	call near	string_get_address
	dd		0x11eeee11		; marker (endianness-insensitive!)
	dd		0xffffffff		; before mark: new SP, after mark: old SP
string_get_address:
	; get address of data
	pop		edx
	; save old ESP and if new has been set by supervisor, set new ESP
	mov		eax, [edx+4]
	mov		[edx+4], esp
	cmp		eax,0xffffffff
	je		set_esp_done
	mov		esp, eax
set_esp_done:
	; NOT the mark
	mov		eax, [edx]
	not		eax
	mov		[edx], eax

; continue with other shellcode
