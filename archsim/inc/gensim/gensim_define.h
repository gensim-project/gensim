#ifndef _DEFINE_H
#define _DEFINE_H

#include "define.h"

/* Always have these. */
//#define UNSIGNED_BITS(v,u,l)  (((uint32_t)(v)<<(31-(u))>>(31-(u)+(l))))
//#define SIGNED_BITS(v,u,l)    (((int32_t)(v)<<(31-(u))>>(31-(u)+(l))))
//#define BITSEL(v,b)           (((v)>>b) & 1UL)
#define BIT_LSB(i) (1 << (i))
#define BIT_32_MSB(i) (0x80000000 >> (i))
#define ac_modifier_decode(x) uint32_t modifier_decode_##x(uint32_t input, Decode insn, uint32_t pc)
#endif /* _DEFINE_H */
