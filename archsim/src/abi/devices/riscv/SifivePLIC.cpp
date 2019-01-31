/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/riscv/SifivePLIC.h"

using namespace archsim::abi::devices::riscv;

using namespace archsim::abi::devices;
static ComponentDescriptor SifivePLICDescriptor ("SifivePLIC", {{"Hart0", ComponentParameter_Thread}});
SifivePLIC::SifivePLIC(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x4000000), IRQController(54), Component(SifivePLICDescriptor)
{
	interrupt_priorities_.resize(GetNumLines());
}

SifivePLIC::~SifivePLIC()
{

}

bool SifivePLIC::Initialise()
{
	return true;
}

bool SifivePLIC::Read(uint32_t offset, uint8_t size, uint32_t& data)
{
	int interrupt_id = (offset & 0xfff) >> 2;

	switch(offset) {
		case 0x0 ... 0x1000:
			data = interrupt_priorities_.at(interrupt_id);
			break;
		default:
			UNIMPLEMENTED;
	}

	return true;
}

bool SifivePLIC::Write(uint32_t offset, uint8_t size, uint32_t data)
{
	int interrupt_id = (offset & 0xfff) >> 2;

	switch(offset) {
		case 0x0 ... 0x1000:
			interrupt_priorities_.at(interrupt_id) = data & 0x7;
			break;
		default:
			UNIMPLEMENTED;
	}

	return true;
}

archsim::core::thread::ThreadInstance* SifivePLIC::GetHart(int i)
{
	ASSERT(i == 0);
	return GetHart0();
}

bool SifivePLIC::AssertLine(uint32_t line)
{
	UNIMPLEMENTED;
	return false;
}

bool SifivePLIC::RescindLine(uint32_t line)
{
	UNIMPLEMENTED;
	return false;
}
