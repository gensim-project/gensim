/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "DiagnosticContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"

#include "ssa/SSATestFixture.h"

using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;

class Issues_Issue5 : public SSATestFixture
{

};

TEST_F(Issues_Issue5, Issue5)
{
	// source code to test

	const std::string sourcecode = R"||(
    
action uint32 bitsel helper (
    uint32 sym_320_3_parameter_val
    uint8 sym_321_3_parameter_bit
  ) [
  ] <
    b_0
  > {
  block b_0 {
    s_b_0_0 = read sym_320_3_parameter_val;
    s_b_0_1 = read sym_321_3_parameter_bit;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary >> s_b_0_0 s_b_0_2;
    s_b_0_4 = constant sint32 1;
    s_b_0_5 = cast uint32 s_b_0_4;
    s_b_0_6 = binary & s_b_0_3 s_b_0_5;
    s_b_0_7: return s_b_0_6;
  }
}
    
action uint32 RRX_C helper (
    uint32 sym_569_3_parameter_x
    uint8 sym_570_3_parameter_carry_in
    uint8& sym_571_3_parameter_carry_out
  ) [
    uint32 sym_583_0_result
  ] <
    b_0
  > {
  block b_0 {
    s_b_0_0 = read sym_570_3_parameter_carry_in;
    s_b_0_1 = cast uint32 s_b_0_0;
    s_b_0_2 = constant sint32 31;
    s_b_0_3 = cast uint32 s_b_0_2;
    s_b_0_4 = binary << s_b_0_1 s_b_0_3;
    s_b_0_5 = read sym_569_3_parameter_x;
    s_b_0_6 = constant sint32 1;
    s_b_0_7 = cast uint32 s_b_0_6;
    s_b_0_8 = binary >> s_b_0_5 s_b_0_7;
    s_b_0_9 = binary | s_b_0_4 s_b_0_8;
    s_b_0_10: write sym_583_0_result s_b_0_9;
    s_b_0_11 = read sym_569_3_parameter_x;
    s_b_0_12 = constant sint32 0;
    s_b_0_13 = cast uint8 s_b_0_12;
    s_b_0_14 = call bitsel s_b_0_11 s_b_0_13;
    s_b_0_15 = cast uint8 s_b_0_14;
    s_b_0_16: write sym_571_3_parameter_carry_out s_b_0_15;
    s_b_0_17 = read sym_583_0_result;
    s_b_0_18: return s_b_0_17;
  }
}
    
action uint32 LSR_C helper (
    uint32 sym_389_3_parameter_bits
    uint32 sym_390_3_parameter_shift
    uint8& sym_391_3_parameter_carry_out
  ) [
  ] <
    b_0
    b_1
    b_2
    b_3
    b_4
    b_5
    b_6
  > {
  block b_0 {
    s_b_0_0 = read sym_390_3_parameter_shift;
    s_b_0_1 = constant sint32 0;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary == s_b_0_0 s_b_0_2;
    s_b_0_4: if s_b_0_3 b_1 b_2;
  }
  block b_1 {
    s_b_1_0 = constant sint32 0;
    s_b_1_1 = cast uint8 s_b_1_0;
    s_b_1_2: write sym_391_3_parameter_carry_out s_b_1_1;
    s_b_1_3 = read sym_389_3_parameter_bits;
    s_b_1_4: return s_b_1_3;
  }
  block b_2 {
    s_b_2_0 = read sym_390_3_parameter_shift;
    s_b_2_1 = constant sint32 32;
    s_b_2_2 = cast uint32 s_b_2_1;
    s_b_2_3 = binary < s_b_2_0 s_b_2_2;
    s_b_2_4: if s_b_2_3 b_3 b_4;
  }
  block b_3 {
    s_b_3_0 = read sym_389_3_parameter_bits;
    s_b_3_1 = read sym_390_3_parameter_shift;
    s_b_3_2 = constant sint32 1;
    s_b_3_3 = cast uint32 s_b_3_2;
    s_b_3_4 = binary - s_b_3_1 s_b_3_3;
    s_b_3_5 = binary >> s_b_3_0 s_b_3_4;
    s_b_3_6 = constant sint32 1;
    s_b_3_7 = cast uint32 s_b_3_6;
    s_b_3_8 = binary & s_b_3_5 s_b_3_7;
    s_b_3_9 = cast uint8 s_b_3_8;
    s_b_3_10: write sym_391_3_parameter_carry_out s_b_3_9;
    s_b_3_11 = read sym_389_3_parameter_bits;
    s_b_3_12 = read sym_390_3_parameter_shift;
    s_b_3_13 = binary >> s_b_3_11 s_b_3_12;
    s_b_3_14: return s_b_3_13;
  }
  block b_4 {
    s_b_4_0 = read sym_390_3_parameter_shift;
    s_b_4_1 = constant sint32 32;
    s_b_4_2 = cast uint32 s_b_4_1;
    s_b_4_3 = binary == s_b_4_0 s_b_4_2;
    s_b_4_4: if s_b_4_3 b_5 b_6;
  }
  block b_5 {
    s_b_5_0 = read sym_389_3_parameter_bits;
    s_b_5_1 = constant uint32 31;
    s_b_5_2 = binary >> s_b_5_0 s_b_5_1;
    s_b_5_3 = cast uint8 s_b_5_2;
    s_b_5_4: write sym_391_3_parameter_carry_out s_b_5_3;
    s_b_5_5 = constant sint32 0;
    s_b_5_6 = cast uint32 s_b_5_5;
    s_b_5_7: return s_b_5_6;
  }
  block b_6 {
    s_b_6_0 = constant sint32 0;
    s_b_6_1 = cast uint8 s_b_6_0;
    s_b_6_2: write sym_391_3_parameter_carry_out s_b_6_1;
    s_b_6_3 = constant sint32 0;
    s_b_6_4 = cast uint32 s_b_6_3;
    s_b_6_5: return s_b_6_4;
  }
}
action uint32 LSR helper (
    uint32 sym_370_3_parameter_x
    uint32 sym_371_3_parameter_shift
  ) [
    uint32 sym_382_0_result
    uint8 sym_387_0_dummy_carry_out
  ] <
    b_0
    b_1
    b_2
    b_3
  > {
  block b_0 {
    s_b_0_0 = read sym_371_3_parameter_shift;
    s_b_0_1 = constant sint32 0;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary == s_b_0_0 s_b_0_2;
    s_b_0_4: if s_b_0_3 b_1 b_3;
  }
  block b_1 {
    s_b_1_0 = read sym_370_3_parameter_x;
    s_b_1_1: write sym_382_0_result s_b_1_0;
    s_b_1_2: jump b_2;
  }
  block b_2 {
    s_b_2_0 = read sym_382_0_result;
    s_b_2_1: return s_b_2_0;
  }
  block b_3 {
    s_b_3_0 = read sym_370_3_parameter_x;
    s_b_3_1 = read sym_371_3_parameter_shift;
    s_b_3_2 = call LSR_C s_b_3_0 s_b_3_1 sym_387_0_dummy_carry_out;
    s_b_3_3: write sym_382_0_result s_b_3_2;
    s_b_3_4: jump b_2;
  }
}

action uint32 LSL_C helper (
    uint32 sym_288_3_parameter_bits
    uint32 sym_289_3_parameter_shift
    uint8& sym_290_3_parameter_carry_out
  ) [
  ] <
    b_0
    b_1
    b_2
    b_3
    b_4
    b_5
    b_6
  > {
  block b_0 {
    s_b_0_0 = read sym_289_3_parameter_shift;
    s_b_0_1 = constant sint32 0;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary == s_b_0_0 s_b_0_2;
    s_b_0_4: if s_b_0_3 b_1 b_2;
  }
  block b_1 {
    s_b_1_0 = constant sint32 0;
    s_b_1_1 = cast uint8 s_b_1_0;
    s_b_1_2: write sym_290_3_parameter_carry_out s_b_1_1;
    s_b_1_3 = read sym_288_3_parameter_bits;
    s_b_1_4: return s_b_1_3;
  }
  block b_2 {
    s_b_2_0 = read sym_289_3_parameter_shift;
    s_b_2_1 = constant sint32 32;
    s_b_2_2 = cast uint32 s_b_2_1;
    s_b_2_3 = binary < s_b_2_0 s_b_2_2;
    s_b_2_4: if s_b_2_3 b_3 b_4;
  }
  block b_3 {
    s_b_3_0 = read sym_288_3_parameter_bits;
    s_b_3_1 = constant sint32 32;
    s_b_3_2 = read sym_289_3_parameter_shift;
    s_b_3_3 = cast uint32 s_b_3_1;
    s_b_3_4 = binary - s_b_3_3 s_b_3_2;
    s_b_3_5 = cast uint8 s_b_3_4;
    s_b_3_6 = call bitsel s_b_3_0 s_b_3_5;
    s_b_3_7 = cast uint8 s_b_3_6;
    s_b_3_8: write sym_290_3_parameter_carry_out s_b_3_7;
    s_b_3_9 = read sym_288_3_parameter_bits;
    s_b_3_10 = cast uint32 s_b_3_9;
    s_b_3_11 = read sym_289_3_parameter_shift;
    s_b_3_12 = binary << s_b_3_10 s_b_3_11;
    s_b_3_13: return s_b_3_12;
  }
  block b_4 {
    s_b_4_0 = read sym_289_3_parameter_shift;
    s_b_4_1 = constant sint32 32;
    s_b_4_2 = cast uint32 s_b_4_1;
    s_b_4_3 = binary == s_b_4_0 s_b_4_2;
    s_b_4_4: if s_b_4_3 b_5 b_6;
  }
  block b_5 {
    s_b_5_0 = read sym_288_3_parameter_bits;
    s_b_5_1 = constant sint32 1;
    s_b_5_2 = cast uint32 s_b_5_1;
    s_b_5_3 = binary & s_b_5_0 s_b_5_2;
    s_b_5_4 = cast uint8 s_b_5_3;
    s_b_5_5: write sym_290_3_parameter_carry_out s_b_5_4;
    s_b_5_6 = constant sint32 0;
    s_b_5_7 = cast uint32 s_b_5_6;
    s_b_5_8: return s_b_5_7;
  }
  block b_6 {
    s_b_6_0 = constant sint32 0;
    s_b_6_1 = cast uint8 s_b_6_0;
    s_b_6_2: write sym_290_3_parameter_carry_out s_b_6_1;
    s_b_6_3 = constant sint32 0;
    s_b_6_4 = cast uint32 s_b_6_3;
    s_b_6_5: return s_b_6_4;
  }
}
    action uint32 LSL helper (
    uint32 sym_269_3_parameter_x
    uint32 sym_270_3_parameter_shift
  ) [
    uint32 sym_281_0_result
    uint8 sym_286_0_dummy_carry_out
  ] <
    b_0
    b_1
    b_2
    b_3
  > {
  block b_0 {
    s_b_0_0 = read sym_270_3_parameter_shift;
    s_b_0_1 = constant sint32 0;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary == s_b_0_0 s_b_0_2;
    s_b_0_4: if s_b_0_3 b_1 b_3;
  }
  block b_1 {
    s_b_1_0 = read sym_269_3_parameter_x;
    s_b_1_1: write sym_281_0_result s_b_1_0;
    s_b_1_2: jump b_2;
  }
  block b_2 {
    s_b_2_0 = read sym_281_0_result;
    s_b_2_1: return s_b_2_0;
  }
  block b_3 {
    s_b_3_0 = read sym_269_3_parameter_x;
    s_b_3_1 = read sym_270_3_parameter_shift;
    s_b_3_2 = call LSL_C s_b_3_0 s_b_3_1 sym_286_0_dummy_carry_out;
    s_b_3_3: write sym_281_0_result s_b_3_2;
    s_b_3_4: jump b_2;
  }
}
action uint32 ROR_C helper (
    uint32 sym_532_3_parameter_x
    uint8 sym_533_3_parameter_shift
    uint8& sym_534_3_parameter_carry_out
  ) [
    uint32 sym_541_0_m
    uint32 sym_553_0_result
  ] <
    b_0
  > {
  block b_0 {
    s_b_0_0 = read sym_533_3_parameter_shift;
    s_b_0_1 = constant sint32 32;
    s_b_0_2 = cast uint32 s_b_0_0;
    s_b_0_3 = cast uint32 s_b_0_1;
    s_b_0_4 = binary % s_b_0_2 s_b_0_3;
    s_b_0_5: write sym_541_0_m s_b_0_4;
    s_b_0_6 = read sym_532_3_parameter_x;
    s_b_0_7 = read sym_541_0_m;
    s_b_0_8 = call LSR s_b_0_6 s_b_0_7;
    s_b_0_9 = read sym_532_3_parameter_x;
    s_b_0_10 = constant sint32 32;
    s_b_0_11 = read sym_541_0_m;
    s_b_0_12 = cast uint32 s_b_0_10;
    s_b_0_13 = binary - s_b_0_12 s_b_0_11;
    s_b_0_14 = call LSL s_b_0_9 s_b_0_13;
    s_b_0_15 = binary | s_b_0_8 s_b_0_14;
    s_b_0_16: write sym_553_0_result s_b_0_15;
    s_b_0_17 = read sym_553_0_result;
    s_b_0_18 = constant sint32 31;
    s_b_0_19 = cast uint8 s_b_0_18;
    s_b_0_20 = call bitsel s_b_0_17 s_b_0_19;
    s_b_0_21 = cast uint8 s_b_0_20;
    s_b_0_22: write sym_534_3_parameter_carry_out s_b_0_21;
    s_b_0_23 = read sym_553_0_result;
    s_b_0_24: return s_b_0_23;
  }
}
   
action uint32 ASR_C helper (
    uint32 sym_20_3_parameter_x
    uint32 sym_21_3_parameter_shift
    uint8& sym_22_3_parameter_carry_out
  ) [
  ] <
    b_0
    b_1
    b_2
    b_3
    b_4
    b_5
    b_6
  > {
  block b_0 {
    s_b_0_0 = constant sint32 0;
    s_b_0_1 = cast uint8 s_b_0_0;
    s_b_0_2: write sym_22_3_parameter_carry_out s_b_0_1;
    s_b_0_3 = read sym_21_3_parameter_shift;
    s_b_0_4 = constant sint32 0;
    s_b_0_5 = cast uint32 s_b_0_4;
    s_b_0_6 = binary == s_b_0_3 s_b_0_5;
    s_b_0_7: if s_b_0_6 b_1 b_2;
  }
  block b_1 {
    s_b_1_0 = read sym_20_3_parameter_x;
    s_b_1_1: return s_b_1_0;
  }
  block b_2 {
    s_b_2_0 = read sym_21_3_parameter_shift;
    s_b_2_1 = constant sint32 32;
    s_b_2_2 = cast uint32 s_b_2_1;
    s_b_2_3 = binary >= s_b_2_0 s_b_2_2;
    s_b_2_4: if s_b_2_3 b_3 b_4;
  }
  block b_3 {
    s_b_3_0 = read sym_20_3_parameter_x;
    s_b_3_1 = cast sint32 s_b_3_0;
    s_b_3_2 = constant sint32 31;
    s_b_3_3 = binary ->> s_b_3_1 s_b_3_2;
    s_b_3_4 = constant sint32 1;
    s_b_3_5 = binary & s_b_3_3 s_b_3_4;
    s_b_3_6 = cast uint8 s_b_3_5;
    s_b_3_7: write sym_22_3_parameter_carry_out s_b_3_6;
    s_b_3_8 = read sym_20_3_parameter_x;
    s_b_3_9 = constant sint64 2147483648;
    s_b_3_10 = cast uint64 s_b_3_8;
    s_b_3_11 = cast uint64 s_b_3_9;
    s_b_3_12 = binary & s_b_3_10 s_b_3_11;
    s_b_3_13: if s_b_3_12 b_5 b_6;
  }
  block b_4 {
    s_b_4_0 = read sym_20_3_parameter_x;
    s_b_4_1 = cast sint32 s_b_4_0;
    s_b_4_2 = read sym_21_3_parameter_shift;
    s_b_4_3 = constant sint32 1;
    s_b_4_4 = cast uint32 s_b_4_3;
    s_b_4_5 = binary - s_b_4_2 s_b_4_4;
    s_b_4_6 = cast uint32 s_b_4_1;
    s_b_4_7 = binary ->> s_b_4_6 s_b_4_5;
    s_b_4_8 = constant sint32 1;
    s_b_4_9 = cast uint32 s_b_4_8;
    s_b_4_10 = binary & s_b_4_7 s_b_4_9;
    s_b_4_11 = cast uint8 s_b_4_10;
    s_b_4_12: write sym_22_3_parameter_carry_out s_b_4_11;
    s_b_4_13 = read sym_20_3_parameter_x;
    s_b_4_14 = cast sint32 s_b_4_13;
    s_b_4_15 = read sym_21_3_parameter_shift;
    s_b_4_16 = cast uint32 s_b_4_14;
    s_b_4_17 = binary ->> s_b_4_16 s_b_4_15;
    s_b_4_18: return s_b_4_17;
  }
  block b_5 {
    s_b_5_0 = constant sint64 4294967295;
    s_b_5_1 = cast uint32 s_b_5_0;
    s_b_5_2: return s_b_5_1;
  }
  block b_6 {
    s_b_6_0 = constant sint32 0;
    s_b_6_1 = cast uint32 s_b_6_0;
    s_b_6_2: return s_b_6_1;
  }
}
    
action uint32 Shift_C helper (
    uint32 sym_605_3_parameter_value
    uint8 sym_606_3_parameter_type
    uint32 sym_607_3_parameter_amount
    uint8 sym_608_3_parameter_carry_in
    uint8& sym_609_3_parameter_carry_out
  ) [
    uint32 sym_620_0_result
  ] <
    b_0
    b_1
    b_2
    b_3
    b_4
    b_5
    b_6
    b_7
    b_8
  > {
  block b_0 {
    s_b_0_0 = read sym_607_3_parameter_amount;
    s_b_0_1 = constant sint32 0;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary == s_b_0_0 s_b_0_2;
    s_b_0_4: if s_b_0_3 b_1 b_2;
  }
  block b_1 {
    s_b_1_0 = read sym_605_3_parameter_value;
    s_b_1_1: write sym_620_0_result s_b_1_0;
    s_b_1_2 = read sym_608_3_parameter_carry_in;
    s_b_1_3: write sym_609_3_parameter_carry_out s_b_1_2;
    s_b_1_4 = read sym_620_0_result;
    s_b_1_5: return s_b_1_4;
  }
  block b_2 {
    s_b_2_0 = read sym_606_3_parameter_type;
    s_b_2_1 = constant sint32 0;
    s_b_2_2 = constant sint32 1;
    s_b_2_3 = constant sint32 2;
    s_b_2_4 = constant sint32 3;
    s_b_2_5 = constant sint32 4;
    s_b_2_6: switch s_b_2_0 b_8 [s_b_2_1 b_3 s_b_2_2 b_4 s_b_2_3 b_5 s_b_2_4 b_6 s_b_2_5 b_7 ];
  }
  block b_3 {
    s_b_3_0 = read sym_605_3_parameter_value;
    s_b_3_1 = read sym_607_3_parameter_amount;
    s_b_3_2 = call LSL_C s_b_3_0 s_b_3_1 sym_609_3_parameter_carry_out;
    s_b_3_3: write sym_620_0_result s_b_3_2;
    s_b_3_4 = read sym_620_0_result;
    s_b_3_5: return s_b_3_4;
  }
  block b_4 {
    s_b_4_0 = read sym_605_3_parameter_value;
    s_b_4_1 = read sym_607_3_parameter_amount;
    s_b_4_2 = call LSR_C s_b_4_0 s_b_4_1 sym_609_3_parameter_carry_out;
    s_b_4_3: write sym_620_0_result s_b_4_2;
    s_b_4_4 = read sym_620_0_result;
    s_b_4_5: return s_b_4_4;
  }
  block b_5 {
    s_b_5_0 = read sym_605_3_parameter_value;
    s_b_5_1 = read sym_607_3_parameter_amount;
    s_b_5_2 = call ASR_C s_b_5_0 s_b_5_1 sym_609_3_parameter_carry_out;
    s_b_5_3: write sym_620_0_result s_b_5_2;
    s_b_5_4 = read sym_620_0_result;
    s_b_5_5: return s_b_5_4;
  }
  block b_6 {
    s_b_6_0 = read sym_605_3_parameter_value;
    s_b_6_1 = read sym_607_3_parameter_amount;
    s_b_6_2 = cast uint8 s_b_6_1;
    s_b_6_3 = call ROR_C s_b_6_0 s_b_6_2 sym_609_3_parameter_carry_out;
    s_b_6_4: write sym_620_0_result s_b_6_3;
    s_b_6_5 = read sym_620_0_result;
    s_b_6_6: return s_b_6_5;
  }
  block b_7 {
    s_b_7_0 = read sym_605_3_parameter_value;
    s_b_7_1 = read sym_608_3_parameter_carry_in;
    s_b_7_2 = call RRX_C s_b_7_0 s_b_7_1 sym_609_3_parameter_carry_out;
    s_b_7_3: write sym_620_0_result s_b_7_2;
    s_b_7_4 = read sym_620_0_result;
    s_b_7_5: return s_b_7_4;
  }
  block b_8 {
    s_b_8_0 = constant sint32 0;
    s_b_8_1 = cast uint32 s_b_8_0;
    s_b_8_2: write sym_620_0_result s_b_8_1;
    s_b_8_3 = constant sint32 0;
    s_b_8_4 = cast uint8 s_b_8_3;
    s_b_8_5: write sym_609_3_parameter_carry_out s_b_8_4;
    s_b_8_6 = read sym_620_0_result;
    s_b_8_7: return s_b_8_6;
  }
}
    
action uint32 Shift helper (
    uint32 sym_594_3_parameter_value
    uint8 sym_595_3_parameter_type
    uint32 sym_596_3_parameter_amount
    uint8 sym_597_3_parameter_carry_in
  ) [
    uint8 sym_603_0_carry_out
    uint32 sym_681_0_result
  ] <
    b_0
  > {
  block b_0 {
    s_b_0_0 = read sym_594_3_parameter_value;
    s_b_0_1 = read sym_595_3_parameter_type;
    s_b_0_2 = read sym_596_3_parameter_amount;
    s_b_0_3 = read sym_597_3_parameter_carry_in;
    s_b_0_4 = call Shift_C s_b_0_0 s_b_0_1 s_b_0_2 s_b_0_3 sym_603_0_carry_out;
    s_b_0_5: write sym_681_0_result s_b_0_4;
    s_b_0_6 = read sym_681_0_result;
    s_b_0_7: return s_b_0_6;
  }
}
    
action void DecodeImmShift helper (
    uint8 sym_147_3_parameter_type
    uint8 sym_148_3_parameter_imm5
    uint8& sym_149_3_parameter_shift_t
    uint8& sym_150_3_parameter_shift_n
  ) [
  ] <
    b_0
    b_1
    b_2
    b_3
    b_4
    b_5
    b_6
    b_7
    b_8
    b_9
    b_10
    b_11
    b_12
    b_13
    b_14
    b_15
    b_16
    b_17
    b_18
    b_19
    b_20
    b_21
  > {
  block b_0 {
    s_b_0_0 = read sym_147_3_parameter_type;
    s_b_0_1 = constant sint32 0;
    s_b_0_2 = cast uint32 s_b_0_0;
    s_b_0_3 = cast uint32 s_b_0_1;
    s_b_0_4 = binary == s_b_0_2 s_b_0_3;
    s_b_0_5: if s_b_0_4 b_1 b_3;
  }
  block b_1 {
    s_b_1_0 = constant sint32 0;
    s_b_1_1 = cast uint8 s_b_1_0;
    s_b_1_2: write sym_149_3_parameter_shift_t s_b_1_1;
    s_b_1_3 = read sym_148_3_parameter_imm5;
    s_b_1_4: write sym_150_3_parameter_shift_n s_b_1_3;
    s_b_1_5: jump b_2;
  }
  block b_2 {
    s_b_2_0: return;
  }
  block b_3 {
    s_b_3_0 = read sym_147_3_parameter_type;
    s_b_3_1 = constant sint32 1;
    s_b_3_2 = cast uint32 s_b_3_0;
    s_b_3_3 = cast uint32 s_b_3_1;
    s_b_3_4 = binary == s_b_3_2 s_b_3_3;
    s_b_3_5: if s_b_3_4 b_4 b_6;
  }
  block b_4 {
    s_b_4_0 = constant sint32 1;
    s_b_4_1 = cast uint8 s_b_4_0;
    s_b_4_2: write sym_149_3_parameter_shift_t s_b_4_1;
    s_b_4_3 = read sym_148_3_parameter_imm5;
    s_b_4_4 = constant sint32 0;
    s_b_4_5 = cast uint32 s_b_4_3;
    s_b_4_6 = cast uint32 s_b_4_4;
    s_b_4_7 = binary == s_b_4_5 s_b_4_6;
    s_b_4_8: if s_b_4_7 b_7 b_9;
  }
  block b_5 {
    s_b_5_0: jump b_2;
  }
  block b_6 {
    s_b_6_0 = read sym_147_3_parameter_type;
    s_b_6_1 = constant sint32 2;
    s_b_6_2 = cast uint32 s_b_6_0;
    s_b_6_3 = cast uint32 s_b_6_1;
    s_b_6_4 = binary == s_b_6_2 s_b_6_3;
    s_b_6_5: if s_b_6_4 b_10 b_12;
  }
  block b_7 {
    s_b_7_0 = constant sint32 32;
    s_b_7_1 = cast uint8 s_b_7_0;
    s_b_7_2: write sym_150_3_parameter_shift_n s_b_7_1;
    s_b_7_3: jump b_8;
  }
  block b_8 {
    s_b_8_0: jump b_5;
  }
  block b_9 {
    s_b_9_0 = read sym_148_3_parameter_imm5;
    s_b_9_1: write sym_150_3_parameter_shift_n s_b_9_0;
    s_b_9_2: jump b_8;
  }
  block b_10 {
    s_b_10_0 = constant sint32 2;
    s_b_10_1 = cast uint8 s_b_10_0;
    s_b_10_2: write sym_149_3_parameter_shift_t s_b_10_1;
    s_b_10_3 = read sym_148_3_parameter_imm5;
    s_b_10_4 = constant sint32 0;
    s_b_10_5 = cast uint32 s_b_10_3;
    s_b_10_6 = cast uint32 s_b_10_4;
    s_b_10_7 = binary == s_b_10_5 s_b_10_6;
    s_b_10_8: if s_b_10_7 b_13 b_15;
  }
  block b_11 {
    s_b_11_0: jump b_5;
  }
  block b_12 {
    s_b_12_0 = read sym_147_3_parameter_type;
    s_b_12_1 = constant sint32 3;
    s_b_12_2 = cast uint32 s_b_12_0;
    s_b_12_3 = cast uint32 s_b_12_1;
    s_b_12_4 = binary == s_b_12_2 s_b_12_3;
    s_b_12_5: if s_b_12_4 b_16 b_18;
  }
  block b_13 {
    s_b_13_0 = constant sint32 32;
    s_b_13_1 = cast uint8 s_b_13_0;
    s_b_13_2: write sym_150_3_parameter_shift_n s_b_13_1;
    s_b_13_3: jump b_14;
  }
  block b_14 {
    s_b_14_0: jump b_11;
  }
  block b_15 {
    s_b_15_0 = read sym_148_3_parameter_imm5;
    s_b_15_1: write sym_150_3_parameter_shift_n s_b_15_0;
    s_b_15_2: jump b_14;
  }
  block b_16 {
    s_b_16_0 = read sym_148_3_parameter_imm5;
    s_b_16_1 = constant sint32 0;
    s_b_16_2 = cast uint32 s_b_16_0;
    s_b_16_3 = cast uint32 s_b_16_1;
    s_b_16_4 = binary == s_b_16_2 s_b_16_3;
    s_b_16_5: if s_b_16_4 b_19 b_21;
  }
  block b_17 {
    s_b_17_0: jump b_11;
  }
  block b_18 {
    s_b_18_0 = constant sint32 0;
    s_b_18_1 = cast uint8 s_b_18_0;
    s_b_18_2: write sym_149_3_parameter_shift_t s_b_18_1;
    s_b_18_3 = constant sint32 0;
    s_b_18_4 = cast uint8 s_b_18_3;
    s_b_18_5: write sym_150_3_parameter_shift_n s_b_18_4;
    s_b_18_6: jump b_17;
  }
  block b_19 {
    s_b_19_0 = constant sint32 4;
    s_b_19_1 = cast uint8 s_b_19_0;
    s_b_19_2: write sym_149_3_parameter_shift_t s_b_19_1;
    s_b_19_3 = constant sint32 1;
    s_b_19_4 = cast uint8 s_b_19_3;
    s_b_19_5: write sym_150_3_parameter_shift_n s_b_19_4;
    s_b_19_6: jump b_20;
  }
  block b_20 {
    s_b_20_0: jump b_17;
  }
  block b_21 {
    s_b_21_0 = constant sint32 3;
    s_b_21_1 = cast uint8 s_b_21_0;
    s_b_21_2: write sym_149_3_parameter_shift_t s_b_21_1;
    s_b_21_3 = read sym_148_3_parameter_imm5;
    s_b_21_4: write sym_150_3_parameter_shift_n s_b_21_3;
    s_b_21_5: jump b_20;
  }
}
    
action uint8 pc_check helper (
    uint32 sym_1943_3_parameter_reg
  ) [
  ] <
    b_0
  > {
  block b_0 {
    s_b_0_0 = read sym_1943_3_parameter_reg;
    s_b_0_1 = constant sint32 15;
    s_b_0_2 = cast uint32 s_b_0_1;
    s_b_0_3 = binary == s_b_0_0 s_b_0_2;
    s_b_0_4 = constant sint32 4;
    s_b_0_5 = constant sint32 0;
    s_b_0_6 = select s_b_0_3 s_b_0_4 s_b_0_5;
    s_b_0_7 = cast uint8 s_b_0_6;
    s_b_0_8: return s_b_0_7;
  }
}
action uint32 AddWithCarry helper (
    uint32 sym_95_3_parameter_op1
    uint32 sym_96_3_parameter_op2
    uint8 sym_97_3_parameter_carry_in
    uint8& sym_98_3_parameter_carry_out
    uint8& sym_99_3_parameter_overflow
  ) [
    uint64 sym_109_0_unsigned_sum
    sint64 sym_121_0_signed_sum
    uint32 sym_127_0_result
  ] <
    b_0
  > {
  block b_0 {
    s_b_0_0 = read sym_95_3_parameter_op1;
    s_b_0_1 = cast uint64 s_b_0_0;
    s_b_0_2 = read sym_96_3_parameter_op2;
    s_b_0_3 = cast uint64 s_b_0_2;
    s_b_0_4 = binary + s_b_0_1 s_b_0_3;
    s_b_0_5 = read sym_97_3_parameter_carry_in;
    s_b_0_6 = cast uint64 s_b_0_5;
    s_b_0_7 = binary + s_b_0_4 s_b_0_6;
    s_b_0_8: write sym_109_0_unsigned_sum s_b_0_7;
    s_b_0_9 = read sym_95_3_parameter_op1;
    s_b_0_10 = cast sint32 s_b_0_9;
    s_b_0_11 = cast sint64 s_b_0_10;
    s_b_0_12 = read sym_96_3_parameter_op2;
    s_b_0_13 = cast sint32 s_b_0_12;
    s_b_0_14 = cast sint64 s_b_0_13;
    s_b_0_15 = binary + s_b_0_11 s_b_0_14;
    s_b_0_16 = read sym_97_3_parameter_carry_in;
    s_b_0_17 = cast sint64 s_b_0_16;
    s_b_0_18 = binary + s_b_0_15 s_b_0_17;
    s_b_0_19: write sym_121_0_signed_sum s_b_0_18;
    s_b_0_20 = read sym_109_0_unsigned_sum;
    s_b_0_21 = constant sint64 4294967295;
    s_b_0_22 = cast uint64 s_b_0_21;
    s_b_0_23 = binary & s_b_0_20 s_b_0_22;
    s_b_0_24 = cast uint32 s_b_0_23;
    s_b_0_25: write sym_127_0_result s_b_0_24;
    s_b_0_26 = read sym_109_0_unsigned_sum;
    s_b_0_27 = constant sint32 32;
    s_b_0_28 = cast uint64 s_b_0_27;
    s_b_0_29 = binary >> s_b_0_26 s_b_0_28;
    s_b_0_30 = constant sint32 0;
    s_b_0_31 = cast uint64 s_b_0_30;
    s_b_0_32 = binary != s_b_0_29 s_b_0_31;
    s_b_0_33: write sym_98_3_parameter_carry_out s_b_0_32;
    s_b_0_34 = read sym_127_0_result;
    s_b_0_35 = cast sint32 s_b_0_34;
    s_b_0_36 = cast sint64 s_b_0_35;
    s_b_0_37 = read sym_121_0_signed_sum;
    s_b_0_38 = binary != s_b_0_36 s_b_0_37;
    s_b_0_39: write sym_99_3_parameter_overflow s_b_0_38;
    s_b_0_40 = read sym_127_0_result;
    s_b_0_41: return s_b_0_40;
  }
}
action void test_instruction (
    Instruction sym_15824_3_parameter_inst
  ) [
    uint8 sym_15831_1_temporary_value
    uint32 sym_15860_0_m
    uint32 sym_15869_0_n
    uint8 sym_15881_0_imm5
    uint8 sym_15886_0_shift_t
    uint8 sym_15887_0_shift_n
    uint32 sym_15895_0_shifted
    uint8 sym_15902_0_carry
    uint8 sym_15903_0_overflow
    uint32 sym_15905_0_result
  ] <
    b_0
    b_1
    b_2
    b_3
    b_4
    b_5
    b_6
    b_7
  > {
  block b_0 {
    s_b_0_0 = struct sym_15824_3_parameter_inst field1;
    s_b_0_1 = constant sint32 15;
    s_b_0_2 = cast uint32 s_b_0_0;
    s_b_0_3 = cast uint32 s_b_0_1;
    s_b_0_4 = binary == s_b_0_2 s_b_0_3;
    s_b_0_5 = constant uint8 0;
    s_b_0_6 = binary != s_b_0_4 s_b_0_5;
    s_b_0_7: write sym_15831_1_temporary_value s_b_0_6;
    s_b_0_8: if s_b_0_6 b_2 b_1;
  }
  block b_1 {
    s_b_1_0 = read sym_15831_1_temporary_value;
    s_b_1_1 = unary ! s_b_1_0;
    s_b_1_2: if s_b_1_1 b_3 b_5;
  }
  block b_2 {
    s_b_2_0 = struct sym_15824_3_parameter_inst field2;
    s_b_2_1 = constant sint32 1;
    s_b_2_2 = cast uint32 s_b_2_0;
    s_b_2_3 = cast uint32 s_b_2_1;
    s_b_2_4 = binary == s_b_2_2 s_b_2_3;
    s_b_2_5 = constant uint8 0;
    s_b_2_6 = binary != s_b_2_4 s_b_2_5;
    s_b_2_7: write sym_15831_1_temporary_value s_b_2_6;
    s_b_2_8: jump b_1;
  }
  block b_3 {
    s_b_3_0 = struct sym_15824_3_parameter_inst field3;
    s_b_3_1 = bankregread 0 s_b_3_0;
    s_b_3_2 = struct sym_15824_3_parameter_inst field3;
    s_b_3_3 = cast uint32 s_b_3_2;
    s_b_3_4 = call pc_check s_b_3_3;
    s_b_3_5 = cast uint32 s_b_3_4;
    s_b_3_6 = binary + s_b_3_1 s_b_3_5;
    s_b_3_7: write sym_15860_0_m s_b_3_6;
    s_b_3_8 = struct sym_15824_3_parameter_inst field2;
    s_b_3_9 = bankregread 0 s_b_3_8;
    s_b_3_10 = struct sym_15824_3_parameter_inst field3;
    s_b_3_11 = cast uint32 s_b_3_10;
    s_b_3_12 = call pc_check s_b_3_11;
    s_b_3_13 = cast uint32 s_b_3_12;
    s_b_3_14 = binary + s_b_3_9 s_b_3_13;
    s_b_3_15: write sym_15869_0_n s_b_3_14;
    s_b_3_16 = struct sym_15824_3_parameter_inst field4;
    s_b_3_17 = cast uint8 s_b_3_16;
    s_b_3_18 = constant sint32 2;
    s_b_3_19 = cast uint32 s_b_3_17;
    s_b_3_20 = cast uint32 s_b_3_18;
    s_b_3_21 = binary << s_b_3_19 s_b_3_20;
    s_b_3_22 = struct sym_15824_3_parameter_inst field1;
    s_b_3_23 = cast uint8 s_b_3_22;
    s_b_3_24 = cast uint32 s_b_3_23;
    s_b_3_25 = binary | s_b_3_21 s_b_3_24;
    s_b_3_26 = cast uint8 s_b_3_25;
    s_b_3_27: write sym_15881_0_imm5 s_b_3_26;
    s_b_3_28 = struct sym_15824_3_parameter_inst field2;
    s_b_3_29 = read sym_15881_0_imm5;
    s_b_3_30 : call DecodeImmShift s_b_3_28 s_b_3_29 sym_15886_0_shift_t sym_15887_0_shift_n;
    s_b_3_31 = read sym_15860_0_m;
    s_b_3_32 = read sym_15886_0_shift_t;
    s_b_3_33 = read sym_15887_0_shift_n;
    s_b_3_34 = cast uint32 s_b_3_33;
    s_b_3_35 = regread 2;
    s_b_3_36 = call Shift s_b_3_31 s_b_3_32 s_b_3_34 s_b_3_35;
    s_b_3_37: write sym_15895_0_shifted s_b_3_36;
    s_b_3_38 = read sym_15869_0_n;
    s_b_3_39 = read sym_15895_0_shifted;
    s_b_3_40 = unary ~ s_b_3_39;
    s_b_3_41 = constant sint32 1;
    s_b_3_42 = cast uint8 s_b_3_41;
    s_b_3_43 = call AddWithCarry s_b_3_38 s_b_3_40 s_b_3_42 sym_15902_0_carry sym_15903_0_overflow;
    s_b_3_44: write sym_15905_0_result s_b_3_43;
    s_b_3_45 = struct sym_15824_3_parameter_inst field1;
    s_b_3_46 = read sym_15905_0_result;
    s_b_3_47: bankregwrite 0 s_b_3_45 s_b_3_46;
    s_b_3_48 = struct sym_15824_3_parameter_inst field2;
    s_b_3_49: if s_b_3_48 b_6 b_7;
  }
  block b_4 {
    s_b_4_0: return;
  }
  block b_5 {
    s_b_5_0: intrinsic 10;
    s_b_5_1: jump b_4;
  }
  block b_6 {
    s_b_6_0 = read sym_15905_0_result;
    s_b_6_1 = constant sint32 31;
    s_b_6_2 = cast uint8 s_b_6_1;
    s_b_6_3 = call bitsel s_b_6_0 s_b_6_2;
    s_b_6_4 = cast uint8 s_b_6_3;
    s_b_6_5: regwrite 4 s_b_6_4;
    s_b_6_6 = read sym_15905_0_result;
    s_b_6_7 = constant sint32 0;
    s_b_6_8 = cast uint32 s_b_6_7;
    s_b_6_9 = binary == s_b_6_6 s_b_6_8;
    s_b_6_10: regwrite 3 s_b_6_9;
    s_b_6_11 = read sym_15902_0_carry;
    s_b_6_12: regwrite 2 s_b_6_11;
    s_b_6_13 = read sym_15903_0_overflow;
    s_b_6_14: regwrite 5 s_b_6_13;
    s_b_6_15: jump b_7;
  }
  block b_7 {
    s_b_7_0: jump b_4;
  }
}
    )||";
	// parse code

	CompileAsm(sourcecode, "test_instruction");

	if(!GetSSACtx()->Resolve(Diag())) {
		std::cout << Diag();
		ASSERT_EQ(true, false);
	}

	// actually perform test
	ASSERT_EQ(true, GetSSACtx()->HasAction("test_instruction"));

	GetSSACtx()->Optimise();
	GetSSACtx()->Resolve(Diag());
}
