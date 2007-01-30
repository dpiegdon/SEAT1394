;
; $Id$

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
