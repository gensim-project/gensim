/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <stdint.h>

extern "C" {
//// output: SZ0A0P1C0000000V
	uint16_t genc_adc_flags(uint32_t lhs, uint32_t rhs, uint8_t carry_in)
	{
		uint64_t result = (uint64_t)lhs + rhs + carry_in;

		uint32_t N = (result & 0x80000000ULL) != 0;
		uint32_t Z = ((uint32_t)result) == 0;
		uint32_t C = result > 0xffffffffULL;

		uint32_t V = ((lhs & 0x80000000) == (rhs & 0x80000000)) && ((lhs & 0x80000000) != (result & 0x80000000));

		return V | C << 8 | Z << 14 | N << 15;
	}

}
