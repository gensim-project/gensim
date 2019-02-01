/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/riscv/SifivePLIC.h"
#include "util/LogContext.h"

UseLogContext(LogDevice);

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

bool SifivePLIC::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	int interrupt_id = (offset & 0xfff) >> 2;

	switch(offset) {
		case 0x0 ... 0x1000:
			data = interrupt_priorities_.at(interrupt_id);
			break;
		case 0x200000:
			data = hart0_m_priority_thresholds_;
			break;
		default:
			LC_DEBUG1(LogDevice) << "Unimplemented PLIC read: " << Address(offset);
			UNIMPLEMENTED;
	}

	return true;
}

bool SifivePLIC::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	int interrupt_id = (offset & 0xfff) >> 2;

	switch(offset) {
		case 0x0 ... 0x1000:
			interrupt_priorities_.at(interrupt_id) = data & 0x7;
			break;
		case 0x200000:
			hart0_m_priority_thresholds_ = data;
			break;
		default:
			LC_DEBUG1(LogDevice) << "Unimplemented PLIC write: " << Address(offset);
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
