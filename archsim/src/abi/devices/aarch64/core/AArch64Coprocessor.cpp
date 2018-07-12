/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/aarch64/core/AArch64Coprocessor.h"
#include "define.h"

using namespace archsim::abi::devices::aarch64::core;

AArch64Coprocessor::AArch64Coprocessor() : tpidr_el0_(0)
{

}


bool AArch64Coprocessor::Initialise()
{
	return true;
}

struct decoded_msr {
	uint8_t op0, op1, op2, crn, crm;
};

decoded_msr DecodeMsr(uint32_t msr)
{
	decoded_msr decoded;
	decoded.op0 = 2 + BITSEL(msr, 19);
	decoded.op1 = UNSIGNED_BITS(msr, 18, 16);
	decoded.op2 = UNSIGNED_BITS(msr, 7, 5);
	decoded.crm = UNSIGNED_BITS(msr, 11, 8);
	decoded.crn = UNSIGNED_BITS(msr, 15, 12);
	return decoded;
}

bool AArch64Coprocessor::Read64(uint32_t address, uint64_t& data)
{
	decoded_msr msr = DecodeMsr(address);

	if(msr.op0 == 3 && msr.op1 == 3 && msr.crn == 13 && msr.crm == 0 && msr.op2 == 2) {
		data = tpidr_el0_;
	} else {
		UNIMPLEMENTED;
		return false;
	}

	return true;
}

bool AArch64Coprocessor::Write64(uint32_t address, uint64_t data)
{
	decoded_msr msr = DecodeMsr(address);

	if(msr.op0 == 3 && msr.op1 == 3 && msr.crn == 13 && msr.crm == 0 && msr.op2 == 2) {
		tpidr_el0_ = data;
	} else {
		UNIMPLEMENTED;
		return false;
	}

	return true;
}
