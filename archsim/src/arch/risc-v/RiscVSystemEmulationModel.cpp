/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/IRQController.h"
#include "arch/risc-v/RiscVSystemEmulationModel.h"
#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"
#include "core/MemoryInterface.h"

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogEmulationModelRiscVSystem, LogEmulationModel, "RISCV-System");

using namespace archsim::abi;
using namespace archsim::arch::riscv;

RiscVSystemEmulationModel::RiscVSystemEmulationModel(int xlen) : archsim::abi::LinuxSystemEmulationModel(xlen == 64), riscv_handle_exception_(nullptr)
{

}

RiscVSystemEmulationModel::~RiscVSystemEmulationModel()
{

}

gensim::DecodeContext* RiscVSystemEmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu)
{
	return new archsim::arch::riscv::RiscVDecodeContext(cpu.GetArch());
}

bool RiscVSystemEmulationModel::Initialise(System& system, archsim::uarch::uArch& uarch)
{
	return true;
}

bool RiscVSystemEmulationModel::PrepareCore(archsim::core::thread::ThreadInstance& core)
{
	return true;
}

ExceptionAction RiscVSystemEmulationModel::HandleException(archsim::core::thread::ThreadInstance* cpu, uint64_t category, uint64_t data)
{
	LC_DEBUG1(LogEmulationModelRiscVSystem) << "Exception " << category << " " << data << " taken at PC " << cpu->GetPC();

	// possibly handle special internal exceptions
	switch(category) {
		case 1024: { // fence.i
			cpu->GetPubsub().Publish(PubSubType::ITlbFullFlush, nullptr);
			cpu->GetPubsub().Publish(PubSubType::DTlbFullFlush, nullptr);
			return ExceptionAction::ResumeNext;
		}
		case 1025: { // fence.vma
			cpu->GetPubsub().Publish(PubSubType::FlushTranslations, nullptr);
			cpu->GetPubsub().Publish(PubSubType::ITlbFullFlush, nullptr);
			cpu->GetPubsub().Publish(PubSubType::DTlbFullFlush, nullptr);
			return ExceptionAction::ResumeNext;
		}
		default: {
			// trigger exception in CPU
			if(riscv_handle_exception_ == nullptr) {
				riscv_handle_exception_ = &cpu->GetArch().GetISA("riscv").GetBehaviours().GetBehaviour("riscv_take_exception");
			}
			riscv_handle_exception_->Invoke(cpu, {category, data});
			return ExceptionAction::AbortInstruction;
		}
	}


}

ExceptionAction RiscVSystemEmulationModel::HandleMemoryFault(archsim::core::thread::ThreadInstance& thread, archsim::MemoryInterface& interface, archsim::Address address)
{
	// make sure we release the lock on the memory interface if we've taken it
	for(auto interface : thread.GetMemoryInterfaces()) {
		interface->GetMonitor()->Unlock(&thread);
	}

	// Exception set up by MMU
	return ExceptionAction::AbortInstruction;
}

void RiscVSystemEmulationModel::HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq)
{
	// Need to check to see if an interrupt is valid
	LC_DEBUG1(LogEmulationModelRiscVSystem) << "Attempting to service an IRQ";
	RiscVSystemCoprocessor *coproc = (RiscVSystemCoprocessor *)thread->GetPeripherals().GetDevice(0);

	if(coproc->CanTakeInterrupt(thread->GetExecutionRing(), irq->Line())) {
		LC_DEBUG1(LogEmulationModelRiscVSystem) << "Interrupt taken at PC " << thread->GetPC();

		// trigger interrupt on CPU
		uint64_t ecause = 0x8000000000000000ULL | (irq->Line());
		HandleException(thread, ecause, 0);
	}
}

