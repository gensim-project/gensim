.text

.extern blkRead8
.extern blkRead16
.extern blkRead32

.globl blkCacheRead8Fallback
blkCacheRead8Fallback:
	push %rax
	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	mov blkRead8@GOTPCREL(%rip), %rax
	callq *%rax

	mov %rax, %r15

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	pop %rax

	retq
.size blkCacheRead8Fallback, .-blkCacheRead8Fallback

.globl blkCacheRead16Fallback
blkCacheRead16Fallback:
	push %rax
	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	mov blkRead16@GOTPCREL(%rip), %rax
	callq *%rax

	mov %rax, %r15

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	pop %rax

	retq
.size blkCacheRead16Fallback, .-blkCacheRead16Fallback

.globl blkCacheRead32Fallback
blkCacheRead32Fallback:
	push %rax
	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	mov blkRead32@GOTPCREL(%rip), %rax
	callq *%rax

	mov %rax, %r15

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	pop %rax

	retq
.size blkCacheRead32Fallback, .-blkCacheRead32Fallback

// WRITE FALLBACKS

.globl blkCacheWrite8Fallback
blkCacheWrite8Fallback:
	push %rax
	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	mov cpuWrite8@GOTPCREL(%rip), %rax
	callq *%rax

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	pop %rax

	retq
.size blkCacheWrite8Fallback, .-blkCacheWrite8Fallback

.globl blkCacheWrite16Fallback
blkCacheWrite16Fallback:
	push %rax
	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	mov cpuWrite16@GOTPCREL(%rip), %rax
	callq *%rax

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	pop %rax

	retq
.size blkCacheWrite16Fallback, .-blkCacheWrite16Fallback

.globl blkCacheWrite32Fallback
blkCacheWrite32Fallback:
	push %rax
	push %rbx
	push %rcx
	push %r8
	push %r9
	push %r10
	push %r11

	mov cpuWrite32@GOTPCREL(%rip), %rax
	callq *%rax

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rcx
	pop %rbx
	pop %rax

	retq
.size blkCacheWrite32Fallback, .-blkCacheWrite32Fallback
