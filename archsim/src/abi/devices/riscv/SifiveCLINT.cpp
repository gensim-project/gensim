/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/thread/ThreadInstance.h"
#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "abi/devices/riscv/SifiveCLINT.h"

using namespace archsim::abi::devices::riscv;

DeclareLogContext(LogRiscVCLINT, "RISCV-CLINT");

using namespace archsim::abi::devices;
static ComponentDescriptor SifiveCLINTDescriptor ("SifiveCLINT", {{"Hart0", ComponentParameter_Thread}, {"Harts", ComponentParameterDescriptor::Container(ComponentParameter_Thread)}});
SifiveCLINT::SifiveCLINT(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x10000), Component(SifiveCLINTDescriptor)
{
	tick_source_ = parent.GetSystem().GetTickSource();
}

SifiveCLINT::~SifiveCLINT()
{

}

bool SifiveCLINT::Initialise()
{
	auto harts = GetHarts();

	for(auto hart : harts) {
		timers_.push_back(new CLINTTimer(hart, this));
	}

	for(auto i : timers_) {
		tick_source_->AddConsumer(*i);
	}

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
			data = GetHartTimer(0)->GetCmp();
			break;
		case 0x4008:
			data = GetHartTimer(1)->GetCmp();
			break;

		case 0xbff8:
			data = GetTimer();
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
			SetMSIP(GetHart(0), data & 1);
			break;
		case 0x4: // MSIP1
			SetMSIP(GetHart(1), data & 1);
			break;
		case 0x8: // MSIP2
		case 0xC: // MSIP3
		case 0x10: // MSIP4
			break;

		case 0x4000:
			LC_DEBUG1(LogRiscVCLINT) << "Wrote MTIMECMP0 <= " << std::hex << data;
			GetHartTimer(0)->SetCmp(data);
			break;
		case 0x4008:
			LC_DEBUG1(LogRiscVCLINT) << "Wrote MTIMECMP1 <= " << std::hex << data;
			GetHartTimer(1)->SetCmp(data);
			break;
		default:
			UNIMPLEMENTED;
	}

	return true;
}

CLINTTimer* SifiveCLINT::GetHartTimer(int i)
{
	return timers_.at(i);
}


archsim::core::thread::ThreadInstance* SifiveCLINT::GetHart(int i)
{
	auto harts = GetHarts();
	if(i >= harts.size()) {
		throw std::out_of_range("Hart index out of range");
	}

	return harts.at(i);
}

archsim::arch::riscv::RiscVSystemCoprocessor* SifiveCLINT::GetCoprocessor(int i)
{
	return static_cast<archsim::arch::riscv::RiscVSystemCoprocessor*>(GetHart(i)->GetPeripherals().GetDevice(0));
}


void SifiveCLINT::SetMSIP(archsim::core::thread::ThreadInstance* thread, bool P)
{
	auto peripheral = thread->GetPeripherals().GetDevice(0);
	auto coproc = (archsim::arch::riscv::RiscVSystemCoprocessor *)peripheral;

	LC_DEBUG1(LogRiscVCLINT) << "Setting MSIP for thread " << 0 << " to " << P;

	if(P) {
		coproc->MachinePendInterrupt(1 << 3);
	} else {
		coproc->MachineUnpendInterrupt(1 << 3);
	}
}

uint64_t SifiveCLINT::GetTimer()
{
	return tick_source_->GetCounter() * 1000;
}


CLINTTimer::CLINTTimer(archsim::core::thread::ThreadInstance* hart, SifiveCLINT* clint) : hart_(hart), clint_(clint), cmp_(-1)
{

}

void CLINTTimer::SetCmp(uint64_t cmp)
{
	cmp_ = cmp;
	CheckTick();
}

uint64_t CLINTTimer::GetCmp() const
{
	return cmp_;
}

void CLINTTimer::CheckTick()
{
	if(clint_->GetTimer() > cmp_) {
		// pend interrupt if it is enabled
		if(clint_->GetCoprocessor(hart_->GetThreadID())->GetMTIE()) {
			clint_->GetCoprocessor(hart_->GetThreadID())->MachinePendInterrupt(1 << 7);
		}

	} else {
		// unpend interrupt
		clint_->GetCoprocessor(hart_->GetThreadID())->MachineUnpendInterrupt(1 << 7);
	}
}


void CLINTTimer::Tick(uint32_t tick_periods)
{
	CheckTick();
}
