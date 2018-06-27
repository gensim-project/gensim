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

.globl genc_adc64_flags
genc_adc64_flags:
	mov %rdi, %r11
	// set carry flag and clear ecx
	movb $0xff, %al
	addb %dl, %al

	// perform add with carry
	adc %rsi, %r11

	// read out flags into correct format
	lahf
	seto %al

	// return
	retq
.size genc_adc64_flags, .-genc_adc64_flags

.globl genc_sbc_flags
genc_sbc_flags:
	ud2	
	retq
.size genc_sbc_flags, .-genc_sbc_flags

.globl genc_sbc64_flags
genc_sbc64_flags:
	mov %rdi, %r11
	// set carry flag and clear ecx
	movb $0xff, %al
	addb %dl, %al

	// perform add with carry
	sbb %rsi, %r11

	// read out flags into correct format
	lahf
	seto %al

	// return
	retq
.size genc_sbc64_flags, .-genc_sbc64_flags

.globl genc_adc
genc_adc:
	ud2	
	retq
.size genc_adc, .-genc_adc

.globl genc_adc64
genc_adc64:
	ud2	
	retq
.size genc_adc64, .-genc_adc64

.globl genc_sbc
genc_sbc:
	ud2	
	retq
.size genc_sbc, .-genc_sbc

.globl genc_sbc64
genc_sbc64:
	ud2
	retq
.size genc_sbc64, .-genc_sbc64