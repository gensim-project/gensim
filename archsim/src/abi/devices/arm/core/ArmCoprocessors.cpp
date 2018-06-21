/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ArmCoprocessors.cpp
 *
 *  Created on: 18 Jun 2014
 *      Author: harry
 */

#include "abi/devices/arm/core/ArmCoprocessor.h"
#include "util/LogContext.h"

#include <cassert>

UseLogContext(LogDevice);
DeclareChildLogContext(LogArmDevice, LogDevice, "ARM");
DeclareChildLogContext(LogArmCoreDevice, LogArmDevice, "Core");

class ArmDebugCoprocessor;
class ArmXScaleElkhart;
class FakeArmFPUCoprocessor;

RegisterComponent(archsim::abi::devices::Device, ArmDebugCoprocessor, "armdebug", "ARMv5 Debug Coprocessor interface");
RegisterComponent(archsim::abi::devices::Device, ArmXScaleElkhart, "elkhart", "XScale Elkhart Debug unit interface");
RegisterComponent(archsim::abi::devices::Device, FakeArmFPUCoprocessor, "fpu", "Fake ARM FPU interface");

using archsim::abi::devices::ArmCoprocessor;

bool ArmCoprocessor::Initialise()
{
	return true;
}

bool ArmCoprocessor::access_cp0(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp1(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp2(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp3(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp4(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp5(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp6(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp7(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp8(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp9(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp10(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp11(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp12(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp13(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp14(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}
bool ArmCoprocessor::access_cp15(bool is_read, uint32_t &data)
{
	assert(!"Unhandled register access");
	return false;
}

bool ArmCoprocessor::Read32(uint32_t addr, uint32_t &data)
{
	assert(addr == 4);

	instrument_read();

	data = 0;
	switch(rn) {
		case 0:
			return access_cp0(true, data);
		case 1:
			return access_cp1(true, data);
		case 2:
			return access_cp2(true, data);
		case 3:
			return access_cp3(true, data);
		case 4:
			return access_cp4(true, data);
		case 5:
			return access_cp5(true, data);
		case 6:
			return access_cp6(true, data);
		case 7:
			return access_cp7(true, data);
		case 8:
			return access_cp8(true, data);
		case 9:
			return access_cp9(true, data);
		case 10:
			return access_cp10(true, data);
		case 11:
			return access_cp11(true, data);
		case 12:
			return access_cp12(true, data);
		case 13:
			return access_cp13(true, data);
		case 14:
			return access_cp14(true, data);
		case 15:
			return access_cp15(true, data);
		default:
			assert(!"Unsupported coprocessor register");
			return false;
	}
}

bool ArmCoprocessor::Write32(uint32_t addr, uint32_t data)
{

	switch (addr) {
		case 0:
			opc1 = data;
			break;
		case 1:
			opc2 = data;
			break;
		case 2:
			rn = data;
			break;
		case 3:
			rm = data;
			break;
		case 4:

			instrument_write();

			switch(rn) {
				case 0:
					return access_cp0(false, data);
				case 1:
					return access_cp1(false, data);
				case 2:
					return access_cp2(false, data);
				case 3:
					return access_cp3(false, data);
				case 4:
					return access_cp4(false, data);
				case 5:
					return access_cp5(false, data);
				case 6:
					return access_cp6(false, data);
				case 7:
					return access_cp7(false, data);
				case 8:
					return access_cp8(false, data);
				case 9:
					return access_cp9(false, data);
				case 10:
					return access_cp10(false, data);
				case 11:
					return access_cp11(false, data);
				case 12:
					return access_cp12(false, data);
				case 13:
					return access_cp13(false, data);
				case 14:
					return access_cp14(false, data);
				case 15:
					return access_cp15(false, data);
				default:
					assert(!"Unsupported coprocessor register");
			}
	}

	return true;
}


class ArmDebugCoprocessor : public ArmCoprocessor
{
	bool Initialise()
	{
		return true;
	}

	bool access_cp0(bool is_read, uint32_t &data)
	{
		if(opc1 == 6) {
			if(opc1 == 6 && rn == 0 && rm == 0 && opc2 == 0) {
				// TEECR
				if(is_read) data = 0;
			}

			return true;
		}

		// Debug ID register
		if(is_read) {
			data = 0x00010000;
			return true;
		}
		return false;
	}

	bool access_cp1(bool is_read, uint32_t &data)
	{
		// Debug status and control register
		if(is_read) {
			// 0000000000000000000000000000
			data = 0;
			return true;
		} else {
			fprintf(stderr, "%c", data & 0xff);
			return true;
		}
	}

	bool access_cp5(bool is_read, uint32_t &data)
	{
		// Data transfer register
		assert(!"Unimplemented");
		return false;
	}

	bool access_cp6(bool is_read, uint32_t &data)
	{
		//watchpoint fault address register
		assert(!"Unimplemented");
		return false;
	}

	bool access_cp7(bool is_read, uint32_t &data)
	{
		//vector catch register
		assert(!"Unimplemented");
		return false;
	}
};

class ArmXScaleElkhart : public ArmCoprocessor
{
	bool Initialise()
	{
		return true;
	}

	bool access_cp0(bool is_read, uint32_t &data)
	{
		switch(rn) {
			case 8:
				fprintf(stderr, "TX reg\n");
				break;
			case 9:
				fprintf(stderr, "RX reg\n");
				break;
			case 10:
				fprintf(stderr, "Debug control and status reg\n");
				break;
			case 11:
				fprintf(stderr, "Trace buffer reg\n");
				break;
			case 12:
				fprintf(stderr, "Checkpoint reg 0\n");
				break;
			case 13:
				fprintf(stderr, "Checkpoint reg 1\n");
				break;
			case 14:
				fprintf(stderr, "TXRX control reg\n");
				break;
			default:
				fprintf(stderr, "Unimplemented %s Elkhart register %u %u %u %u 0x%08x\n", is_read? "read" : "write", rn, rm, opc1, opc2, data);
				return false;
		}
		return true;
	}
};

class FakeArmFPUCoprocessor : public ArmCoprocessor
{
	bool access_cp0(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}

	bool access_cp1(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp2(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp3(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp4(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp5(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp6(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp7(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp8(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}
	bool access_cp9(bool is_read, uint32_t &data)
	{
		if (is_read)
			data = 0;
		return true;
	}

};
