/*
 * gensim_processor_api.S
 *
 *  Created on: 21 Jul 2015
 *      Author: harry
 */

.text

// %rdi: lhs
// %rsi: rhs
// %rdx: carry_in
// output: SZ0A0P1C0000000V

.globl genc_adc_flags
genc_adc_flags:
	mov %edi, %r11d
	// set carry flag and clear ecx
	movb $0xff, %al
	addb %dl, %al

	// perform add with carry
	adc %esi, %r11d

	// read out flags into correct format
	lahf
	seto %al

	// return
	retq

.size genc_adc_flags, .-genc_adc_flags

.extern DefaultReadHandler
.extern DefaultTxlnHandler
.extern DefaultWriteHandler

DefaultReadHandlerSave:
// AX - return reg
	push %rbx
	push %rcx
// DX - return reg
	push %r8
	push %r9
	push %r10
	push %r11
	callq DefaultReadHandler
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	retq

.globl DefaultWriteHandlerSave
DefaultWriteHandlerSave:
// AX - return reg
	push %rbx
	push %rcx
// DX - return reg
	push %r8
	push %r9
	push %r10
	push %r11
	callq DefaultWriteHandler
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	retq


.globl DefaultReadHandlerShunt
DefaultReadHandlerShunt:
	jmp DefaultReadHandlerSave
	.org DefaultReadHandlerShunt+10
	jmp DefaultTxlnHandler

.size DefaultReadHandlerShunt, .-DefaultReadHandlerShunt
