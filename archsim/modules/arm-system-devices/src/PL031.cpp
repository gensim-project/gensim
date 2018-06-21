/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "PL031.h"
#include "util/LogContext.h"

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogPL031, LogArmExternalDevice, "PL031");

using namespace archsim::abi::devices;
using namespace archsim::abi::external::arm;

static ComponentDescriptor pl031_descriptor ("PL031");
PL031::PL031(EmulationModel& parent_model, Address base_address) : PrimecellRegisterDevice(parent_model, base_address, 0x1000, 0x00141031, 0xb105f00d, "pl031"), Component(pl031_descriptor)
{

}

PL031::~PL031()
{

}

bool PL031::Initialise()
{
	return true;
}

uint32_t PL031::ReadRegister(MemoryRegister& reg)
{
	LC_DEBUG1(LogPL031) << "Read Register: " << reg.GetName() << " = " << reg.Read();
	return PrimecellRegisterDevice::ReadRegister(reg);
}

void PL031::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	LC_DEBUG1(LogPL031) << "Write Register: " << reg.GetName() << " = " << std::hex << value << " (old = " << reg.Read() << ")";
	PrimecellRegisterDevice::WriteRegister(reg, value);
}
