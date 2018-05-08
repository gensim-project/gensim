/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdint.h>

//// output: SZ0A0P1C0000000V
uint16_t genc_adc_flags(uint32_t lhs, uint32_t rhs, uint8_t carry_in)
{
	uint64_t result = (uint64_t)lhs + rhs + carry_in;
	
	uint32_t N = result & 0x80000000;
	uint32_t Z = result == 0;
	uint32_t C = result > 0xffffffff;
	
	uint32_t V = ((lhs & 0x80000000) == (rhs & 0x80000000)) && ((lhs & 0x80000000) != (result & 0x80000000));
	
	return V | C << 8 | Z << 14 | N << 15;
}