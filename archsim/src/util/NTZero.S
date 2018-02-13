.text

// Zero out a region of memory using non-temporal stores
// Parameters:
// RDI: Pointer (must be 8-byte aligned)
// ESI: Size in bytes (must be multiple of 8)
.globl NTZero
NTZero:
#ifndef NDEBUG
// Check some assertions - pointer must be 8-byte aligned, size must be multiple of 8
	mov %rdi, %rax
	and $0x7, %rax
	jnz assert_fail

	mov %esi, %eax
	and $0x7, %eax
	jnz assert_fail

	jmp 1f

assert_fail:
	ud2

1:
#endif

	xor %rax, %rax
.align 8
1:
	movnti %rax, (%rdi)

	sub $8, %esi
	add $8, %rdi
	test %esi, %esi
	jnz 1b
2:
	retq
.size NTZero, .-NTZero
