/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/thread/ThreadInstance.h"
#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "abi/devices/riscv/SifiveCLINT.h"

using namespace archsim::abi::devices::riscv;


using namespace archsim::abi::devices;
static ComponentDescriptor SifiveCLINTDescriptor ("SifiveCLINT", {{"Hart0", ComponentParameter_Thread}});
SifiveCLINT::SifiveCLINT(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x10000), Component(SifiveCLINTDescriptor)
{

}

SifiveCLINT::~SifiveCLINT()
{

}

bool SifiveCLINT::Initialise()
{
	return true;
}

bool SifiveCLINT::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	switch(offset) {
		case 0x0: // MSIP0
		case 0x4: // MSIP1
		case 0x8: // MSIP2
		case 0xC: // MSIP3
		case 0x10: // MSIP4
			data = 0;
			break;

		case 0x4000:
			data = mtimecmp0;
			break;
		default:
			UNIMPLEMENTED;
	}

	return true;
}

bool SifiveCLINT::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	switch(offset) {
		case 0x0: // MSIP0
			SetMIP(GetHart(0), data & 1);
			break;
		case 0x4: // MSIP1
		case 0x8: // MSIP2
		case 0xC: // MSIP3
		case 0x10: // MSIP4
			break;

		case 0x4000:
			mtimecmp0 = data;
			break;
		default:
			UNIMPLEMENTED;
	}

	return true;
}

archsim::core::thread::ThreadInstance* SifiveCLINT::GetHart(int i)
{
	ASSERT(i == 0);
	return GetHart0();
}

void SifiveCLINT::SetMIP(archsim::core::thread::ThreadInstance* thread, bool P)
{
	auto peripheral = thread->GetPeripherals().GetDevice(0);
	auto coproc = (archsim::arch::riscv::RiscVSystemCoprocessor *)peripheral;
	coproc->MachinePendInterrupt(1 << 3);
}
