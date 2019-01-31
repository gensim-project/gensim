/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/SerialPort.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "arch/risc-v/RiscVMMU.h"
#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "arch/risc-v/RiscVSifiveFU540EmulationModel.h"
#include "abi/devices/riscv/SifiveCLINT.h"
#include "abi/devices/riscv/SifivePLIC.h"
#include "abi/devices/riscv/SifiveUART.h"
#include "module/ModuleManager.h"
#include "util/ComponentManager.h"
#include "system.h"

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogEmulationModelRiscVSystem, LogEmulationModel, "RISCV-System");

using namespace archsim::abi;
using namespace archsim::arch::riscv;

class RVSystemEmulationModel32 : public RiscVSifiveFU540EmulationModel
{
public:
	RVSystemEmulationModel32() : RiscVSifiveFU540EmulationModel(32) {}
};

class RVSystemEmulationModel64 : public RiscVSifiveFU540EmulationModel
{
public:
	RVSystemEmulationModel64() : RiscVSifiveFU540EmulationModel(64) {}
};



RiscVSifiveFU540EmulationModel::RiscVSifiveFU540EmulationModel(int xlen) : LinuxSystemEmulationModel(xlen==64)
{

}

gensim::DecodeContext* RiscVSifiveFU540EmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu)
{
	return new archsim::arch::riscv::RiscVDecodeContext(cpu.GetArch());
}

bool RiscVSifiveFU540EmulationModel::Initialise(System& system, archsim::uarch::uArch& uarch)
{
	if (!SystemEmulationModel::Initialise(system, uarch))
		return false;

	// Initialise "physical memory".
	if (!GetMemoryModel().GetMappingManager()) {
		return false;
	}

	GetMemoryModel().GetMappingManager()->MapRegion(Address(0x10000), 0x8000, archsim::abi::memory::RegFlagReadWriteExecute, "ROM");
	GetMemoryModel().GetMappingManager()->MapRegion(Address(0x80000000), 512 * 1024 * 1024, archsim::abi::memory::RegFlagReadWriteExecute, "DRAM");

	// load device tree
	if(archsim::options::DeviceTreeFile.IsSpecified()) {
		std::ifstream str(archsim::options::DeviceTreeFile.GetValue(), std::ios::binary | std::ios::ate);
		auto size = str.tellg();
		std::vector<char> data (size);
		str.seekg(0);
		str.read(data.data(), size);

		GetMemoryModel().Write(Address(0x10000), (uint8_t*)data.data(), size);
	}

	return true;
}


void RiscVSifiveFU540EmulationModel::DestroyDevices()
{

}

ExceptionAction RiscVSifiveFU540EmulationModel::HandleException(archsim::core::thread::ThreadInstance* cpu, uint64_t category, uint64_t data)
{
	// trigger exception in CPU
	cpu->GetArch().GetISA("riscv").GetBehaviours().GetBehaviour("riscv_take_exception").Invoke(cpu, {category, data});
	return ExceptionAction::AbortInstruction;
}

ExceptionAction RiscVSifiveFU540EmulationModel::HandleMemoryFault(archsim::core::thread::ThreadInstance& thread, archsim::MemoryInterface& interface, archsim::Address address)
{
	// Exception set up by MMU
	return ExceptionAction::AbortInstruction;
}

void RiscVSifiveFU540EmulationModel::HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq)
{
	LC_DEBUG1(LogEmulationModelRiscVSystem) << "Interrupt taken at PC " << thread->GetPC();

	// trigger interrupt on CPU
	uint64_t ecause = 0x8000000000000000ULL | (irq->Line());
	HandleException(thread, ecause, 0);
}


bool RiscVSifiveFU540EmulationModel::InstallDevices()
{
	return InstallCoreDevices() && InstallPlatformDevices();
}

bool RiscVSifiveFU540EmulationModel::InstallCoreDevices()
{
	auto mmu = new archsim::arch::riscv::RiscVMMU();
	auto coprocessor = new archsim::arch::riscv::RiscVSystemCoprocessor(main_thread_, mmu);

	main_thread_->GetPeripherals().RegisterDevice("mmu", mmu);
	main_thread_->GetPeripherals().RegisterDevice("coprocessor", coprocessor);

	main_thread_->GetPeripherals().AttachDevice("coprocessor", 0);


	return true;
}

bool RiscVSifiveFU540EmulationModel::InstallPlatformDevices()
{
	using namespace archsim::module;

	// CLINT
	auto clint = new archsim::abi::devices::riscv::SifiveCLINT(*this, Address(0x2000000));
	clint->SetParameter("Hart0", main_thread_);
	RegisterMemoryComponent(*clint);

	// PLIC
	auto plic = new archsim::abi::devices::riscv::SifivePLIC(*this, Address(0x0c000000));
	plic->SetParameter("Hart0", main_thread_);
	RegisterMemoryComponent(*plic);

	auto uart = new archsim::abi::devices::riscv::SifiveUART(*this, Address(0x10010000));
	uart->SetParameter("SerialPort", new archsim::abi::devices::ConsoleOutputSerialPort());
	RegisterMemoryComponent(*uart);

	return true;
}

bool RiscVSifiveFU540EmulationModel::InstallPlatform(loader::BinaryLoader& loader)
{
//	if (!LinuxSystemEmulationModel::InstallPlatform(loader))
//		return false;

	return InstallBootloader(loader);
}

bool RiscVSifiveFU540EmulationModel::InstallBootloader(loader::BinaryLoader &loader)
{
	return true;
}

bool RiscVSifiveFU540EmulationModel::PrepareCore(archsim::core::thread::ThreadInstance& core)
{
	// invoke reset exception
	core.GetArch().GetISA("riscv").GetBehaviours().GetBehaviour("riscv_reset").Invoke(&core, {});

	// run 'bootloader'
	auto pc_ptr = core.GetRegisterFileInterface().GetEntry<uint64_t>("PC");
	*pc_ptr = 0x80000000;

	auto &a0 = core.GetRegisterFileInterface().GetEntry<uint64_t>("GPR")[10];
	auto &a1 = core.GetRegisterFileInterface().GetEntry<uint64_t>("GPR")[11];
	a0 = 0x10000;
	a1 = 0x10000;

	return true;
}

RegisterComponent(archsim::abi::EmulationModel, RVSystemEmulationModel64, "riscv64-sifive", "SiFive FU540-C000");
