.text

.globl rt_sigreturn
rt_sigreturn:
	li a0, -1
	ret

.globl gettimeofday
gettimeofday:
	li a0, -1
	ret

.globl clock_gettime
clock_gettime:
	li a0, -1
	ret	

.globl clock_getres
clock_getres:
	li a0, -1
	ret

.globl getcpu
getcpu:
	li a0, -1
	ret

.globl flush_icache
flush_icache:
	li a0, -1
	ret
