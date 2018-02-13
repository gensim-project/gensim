.data

#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)

.globl eabi_helpers_start
eabi_helpers_start:
.incbin STRINGIFY2(EABIHELPERSFILE)

.globl eabi_helpers_end
eabi_helpers_end:

.globl eabi_helpers_size
eabi_helpers_size: .word eabi_helpers_end - eabi_helpers_start
