#include "blockjit/blockcache-defines.h"

.text

.globl block_trampoline_chain
block_trampoline_chain:
	push %r15
	push %r14
	push %r13
	push %r12
	push %rbx

	// stack alignment
	push $0x0
	push %rsi
	push %rbp

	// Load CPUSTATE and REGSTATE values into their respective regs
	mov %rdi, %r12
	mov 0x8(%rdi), %rbp

1:
	// Load the virtual profile from the stack
	mov 0x8(%rsp), %rsi

	// Load the guest PC
	// XXX ARM HAX
	mov 0x080(%rbp), %eax
	mov %eax, %ebx

        // Get the index to look up in the cache
        // This and operation should give us the correct cache index, multiplied by the instruction size
	and $BLOCKCACHE_MASK, %ebx

        // Then the scale on the cmp and callq should give us the correct cache entry size of 16 bytes
	cmp %eax, (%rsi, %rbx, 8)
	jne 2f

	callq *0x8(%rsi, %rbx, 8)

	// Check for interrupts
	cmpl $0x0, 0x18(%r12)
        jz 1b

	// Check iteration count
	//mov 0x14(%r12), %esi
	//test %esi, %esi
	//jmp 1b
2:

	pop %rbp

	pop %rsi

	// stack alignment
	pop %rsi

	pop %rbx
	pop %r12
	pop %r13
	pop %r14
	pop %r15
	retq
.size block_trampoline_chain, .-block_trampoline_chain

.globl block_trampoline
block_trampoline:
	push %r15
	push %r14
	push %r13
	push %r12
	push %rbx

	// stack alignment
	push $0x0
	push %rsi
	push %rbp

	// Load CPUSTATE and REGSTATE values into their respective regs
	mov %rdi, %r12
	mov 0x8(%rdi), %rbp

	callq *%rsi

	pop %rbp
	pop %rsi

	// stack alignment
	pop %rsi

	pop %rbx
	pop %r12
	pop %r13
	pop %r14
	pop %r15
	retq
.size block_trampoline, .-block_trampoline
