BITS 32

	call		string_get_address
	;                1234567890123456789012345678901
	db		'foobar23 - marked by SEAT1394', 0xd, 0xa, 0
string_get_address:
	pop		esi
	xor		eax,eax

	mov		ebx,1				; stdout as FD
	mov		al, 4				; write
	mov		ecx,esi				; string-address
	mov		dx,0x20				; size of string
	int		0x80

	mov		al,6				; close
	int		0x80

;	mov		al,0x01				; exit (sigsegv?!)
;	xor		ebx,ebx				; exit code
;	int		0x80

	ret
