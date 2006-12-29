BITS 32

	call		string_get_address
	;                1234567890123456789012345678901
	db		0x00,0xff,0xff,0x00
string_get_address:
	pop		edx
	mov		eax, [edx]
	not		eax
	mov		[edx], eax

; continue with other shellcode
