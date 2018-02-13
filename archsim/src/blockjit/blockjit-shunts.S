.text

/*

REGULAR FUNCTION MEMORY READ FALLBACKS

*/

.extern blkRead8
.extern blkRead16
.extern blkRead32
.extern blkFnFallbackSlot

.align 8
.globl blkFnRead8Fallback
blkFnRead8Fallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkRead8
	movzbl %al, %eax
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	mov %eax, (%rdx)

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.globl blkFnRead16Fallback
blkFnRead16Fallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkRead16
	movzwl %ax, %eax
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	mov %eax, (%rdx)

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.globl blkFnRead32Fallback
blkFnRead32Fallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkRead32
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	mov %eax, (%rdx)

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

/*

USER PERMISSIONS FUNCTION MEMORY READ FALLBACKS

*/

.align 8
.globl blkFnRead8UserFallback
blkFnRead8UserFallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkRead8User
	movzbl %al, %eax
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	mov %eax, (%rdx)

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.globl blkFnRead16UserFallback
blkFnRead16UserFallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkRead16User
	movzwl %ax, %eax
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	mov %eax, (%rdx)

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.globl blkFnRead32UserFallback
blkFnRead32UserFallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkRead32User
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	mov %eax, (%rdx)

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.extern blkWrite8
.extern blkWrite16
.extern blkWrite32
.extern blkFnFallbackSlot
.globl blkFnWrite8Fallback
blkFnWrite8Fallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi
	// Value in %rdx

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkWrite8
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx
	
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.globl blkFnWrite16Fallback
blkFnWrite16Fallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi
	// Value in %rdx

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkWrite16

	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

.align 8

.globl blkFnWrite32Fallback
blkFnWrite32Fallback:
	// CPU STATE pointer in %rdi
	// Address in %rsi
	// Value in %rdx

	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	callq blkWrite32
	
	lea blkFnFallbackSlotAddr(%rip), %rdx
	mov (%rdx), %rdx

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx

	retq

blkFnFallbackSlotAddr:
.quad blkFnFallbackSlot
