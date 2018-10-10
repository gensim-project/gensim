
.text

.globl clock_gettime
clock_gettime:
	xor %rax, %rax
	dec %rax
	retq

.globl gettimeofday
gettimeofday:
	xor %rax, %rax
	dec %rax	
	retq

.globl time
time:
	xor %rax, %rax
	dec %rax	
	retq

.globl getcpu
getcpu:
	xor %rax, %rax
	dec %rax	
	retq

