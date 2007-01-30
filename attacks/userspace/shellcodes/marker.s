;
; $Id$

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
