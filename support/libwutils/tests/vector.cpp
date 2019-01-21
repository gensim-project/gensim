/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <gtest/gtest.h>
#include "wutils/Vector.h"

TEST(Archsim_Vector, ExtendBoolean)
{
	wutils::Vector<bool, 1> b;
	b.InsertElement(0, true);

	wutils::Vector<uint32_t, 1> w (b);
	ASSERT_EQ(w.ExtractElement(0), 0xffffffff);
}

TEST(Archsim_Vector, CompareAndExtend)
{
	wutils::Vector<double, 1> d0 {0.0}, d1 {0.0};

	{
		wutils::Vector<bool, 1> result = d0 == d1;
		wutils::Vector<uint8_t, 1> result_u8 (result);
		wutils::Vector<uint16_t, 1> result_u16 (result);
		wutils::Vector<uint32_t, 1> result_u32 (result);

		ASSERT_EQ(result.ExtractElement(0), true);
		ASSERT_EQ(result_u8.ExtractElement(0), 0xff);
		ASSERT_EQ(result_u16.ExtractElement(0), 0xffff);
		ASSERT_EQ(result_u32.ExtractElement(0), 0xffffffff);
	}

	{
		wutils::Vector<bool, 1> result = d0 == d1;
		ASSERT_EQ(result.ExtractElement(0), true);
	}
	{
		wutils::Vector<uint8_t, 1> result_u8 = d0 == d1;
		ASSERT_EQ(result_u8.ExtractElement(0), 0xff);
	}
	{
		wutils::Vector<uint16_t, 1> result_u16 = d0 == d1;
		ASSERT_EQ(result_u16.ExtractElement(0), 0xffff);
	}
	{
		wutils::Vector<uint32_t, 1> result_u32 = d0 == d1;
		ASSERT_EQ(result_u32.ExtractElement(0), 0xffffffff);
	}
}

TEST(Archsim_Vector, IntegerOperator)
{
	wutils::Vector<uint32_t, 4> v1, v2;
	v1 = {1, 2, 3, 4};
	v2 = {1, 2, 3, 4};

	auto v3 = v1 + v2;
	wutils::Vector<uint32_t, 4> expected = {2,4,6,8};

	ASSERT_EQ(v3.ExtractElement(0), 2);
	ASSERT_EQ(v3.ExtractElement(1), 4);
	ASSERT_EQ(v3.ExtractElement(2), 6);
	ASSERT_EQ(v3.ExtractElement(3), 8);
}
TEST(Archsim_Vector, IntegerOperateAndSet)
{
	wutils::Vector<uint32_t, 4> v1, v2;
	v1 = {1, 2, 3, 4};
	v2 = {1, 2, 3, 4};

	auto v3 = v1;
	v3 += v2;
	wutils::Vector<uint32_t, 4> expected = {2,4,6,8};

	ASSERT_EQ(v3.ExtractElement(0), 2);
	ASSERT_EQ(v3.ExtractElement(1), 4);
	ASSERT_EQ(v3.ExtractElement(2), 6);
	ASSERT_EQ(v3.ExtractElement(3), 8);
}
