#include "PrimecellRegisterDevice.h"

using namespace archsim::abi::devices;
using namespace archsim::abi::external::arm;

PrimecellRegisterDevice::PrimecellRegisterDevice(EmulationModel& parent_model, Address base_address, uint32_t size, uint32_t periph_id, uint32_t primecell_id, std::string name)
	: RegisterBackedMemoryComponent(parent_model, base_address, size, name),
	  PeriphID0("PeriphID0", 0xfe0, 8, (uint32_t)((periph_id >> 0) & 0xff)),
	  PeriphID1("PeriphID1", 0xfe4, 8, (uint32_t)((periph_id >> 8) & 0xff)),
	  PeriphID2("PeriphID2", 0xfe8, 8, (uint32_t)((periph_id >> 16) & 0xff)),
	  PeriphID3("PeriphID3", 0xfec, 8, (uint32_t)((periph_id >> 24) & 0xff)),
	  PrimecellID0("PrimecellID0", 0xff0, 8, (uint32_t)((primecell_id >> 0) & 0xff)),
	  PrimecellID1("PrimecellID1", 0xff4, 8, (uint32_t)((primecell_id >> 8) & 0xff)),
	  PrimecellID2("PrimecellID2", 0xff8, 8, (uint32_t)((primecell_id >> 16) & 0xff)),
	  PrimecellID3("PrimecellID3", 0xffc, 8, (uint32_t)((primecell_id >> 24) & 0xff))

{
	AddRegister(PeriphID0);
	AddRegister(PeriphID1);
	AddRegister(PeriphID2);
	AddRegister(PeriphID3);
	AddRegister(PrimecellID0);
	AddRegister(PrimecellID1);
	AddRegister(PrimecellID2);
	AddRegister(PrimecellID3);
}

uint32_t PrimecellRegisterDevice::ReadRegister(MemoryRegister& reg)
{
	return RegisterBackedMemoryComponent::ReadRegister(reg);
}

void PrimecellRegisterDevice::WriteRegister(MemoryRegister& reg, uint32_t value)
{
	RegisterBackedMemoryComponent::WriteRegister(reg, value);
}
