/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>
#include <random>

#include "gensim/gensim_processor_api.h"

#define ITERATIONS 10000

class GensimProcessorAPI : public ::testing::Test
{
public:
	std::mt19937_64 Random;
};

TEST_F(GensimProcessorAPI, Adc)
{
	for(int i = 0; i < ITERATIONS; ++i) {
		uint32_t a = Random(), b = Random(), c = Random() % 2;
		uint32_t result = a + b + c;
		ASSERT_EQ(genc_adc(a, b, c), result);
	}
}

TEST_F(GensimProcessorAPI, Adc64)
{
	for(int i = 0; i < ITERATIONS; ++i) {
		uint64_t a = Random(), b = Random(), c = Random() % 2;
		uint64_t result = a + b + c;
		ASSERT_EQ(genc_adc64(a, b, c), result);
	}
}

TEST_F(GensimProcessorAPI, AdcFlags)
{
	for(int i = 0; i < ITERATIONS; ++i) {
		uint32_t a = Random(), b = Random(), c = Random() % 2;
		uint64_t result = a + b + c;

		uint8_t should_be_zero = (a + b + c) == 0;
		uint8_t should_be_negative = (a + b + c) > 0x7fffffff;
		uint8_t should_be_carry = ((uint64_t)a + b + c) > (a + b + c);
		uint8_t should_be_overflow = ((a & 0x80000000) == (b & 0x80000000)) && ((a & 0x80000000) != (result & 0x80000000));

		//SZ0A0P1C0000000V
		uint16_t flags = genc_adc_flags(a, b, c);
		uint8_t is_negative = flags >> 15;
		uint8_t is_zero = (flags >> 14) & 1;
		uint8_t is_carry = (flags >> 8) & 1;
		uint8_t is_overflow = flags & 1;

		ASSERT_EQ(should_be_zero, is_zero);
		ASSERT_EQ(should_be_negative, is_negative);
		ASSERT_EQ(should_be_carry, is_carry);
		ASSERT_EQ(should_be_overflow, is_overflow);
	}
}

TEST_F(GensimProcessorAPI, Adc64Flags)
{

}

TEST_F(GensimProcessorAPI, Sbc)
{
	for(int i = 0; i < ITERATIONS; ++i) {
		uint32_t a = Random(), b = Random(), c = Random() % 2;
		uint32_t result = a - (b + c);
		ASSERT_EQ(genc_sbc(a, b, c), result);
	}
}

TEST_F(GensimProcessorAPI, Sbc64)
{
	for(int i = 0; i < ITERATIONS; ++i) {
		uint64_t a = Random(), b = Random(), c = Random() % 2;
		uint64_t result = a - (b + c);
		ASSERT_EQ(genc_sbc64(a, b, c), result);
	}
}

TEST_F(GensimProcessorAPI, SbcFlags)
{

}

TEST_F(GensimProcessorAPI, Sbc64Flags)
{

}