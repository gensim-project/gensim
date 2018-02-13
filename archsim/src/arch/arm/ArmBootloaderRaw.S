.data

#define STRINGIFY(x) #x
#define STRINGIFY2(x) STRINGIFY(x)

.globl bootloader_start
bootloader_start:
.incbin STRINGIFY2(ARMBOOTLOADERBINFILE)

.globl bootloader_end
bootloader_end:

.globl bootloader_size
bootloader_size: .word bootloader_end - bootloader_start
