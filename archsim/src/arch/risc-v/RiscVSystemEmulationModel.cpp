/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/IRQController.h"
#include "arch/risc-v/RiscVSystemEmulationModel.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogEmulationModelRiscVSystem, LogEmulationModel, "RISCV-System");

using namespace archsim::abi;
using namespace archsim::arch::riscv;

RiscVSystemEmulationModel::RiscVSystemEmulationModel(int xlen) : archsim::abi::LinuxSystemEmulationModel(xlen == 64)
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

	// trigger exception in CPU
	cpu->GetArch().GetISA("riscv").GetBehaviours().GetBehaviour("riscv_take_exception").Invoke(cpu, {category, data});
	return ExceptionAction::AbortInstruction;
}

ExceptionAction RiscVSystemEmulationModel::HandleMemoryFault(archsim::core::thread::ThreadInstance& thread, archsim::MemoryInterface& interface, archsim::Address address)
{
	// Exception set up by MMU
	return ExceptionAction::AbortInstruction;
}

void RiscVSystemEmulationModel::HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq)
{
	LC_DEBUG1(LogEmulationModelRiscVSystem) << "Interrupt taken at PC " << thread->GetPC();

	// trigger interrupt on CPU
	uint64_t ecause = 0x8000000000000000ULL | (irq->Line());
	HandleException(thread, ecause, 0);
}

