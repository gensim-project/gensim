/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/SerialPort.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "arch/risc-v/RiscVMMU.h"
#include "arch/risc-v/RiscVSystemEmulationModel.h"
#include "module/ModuleManager.h"
#include "util/ComponentManager.h"
#include "system.h"

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogEmulationModelRiscVSystem, LogEmulationModel, "RISCV-System");

using namespace archsim::abi;
using namespace archsim::arch::riscv;

class RVSystemEmulationModel32 : public RiscVSystemEmulationModel
{
public:
	RVSystemEmulationModel32() : RiscVSystemEmulationModel(32) {}
};

class RVSystemEmulationModel64 : public RiscVSystemEmulationModel
{
public:
	RVSystemEmulationModel64() : RiscVSystemEmulationModel(64) {}
};



RiscVSystemEmulationModel::RiscVSystemEmulationModel(int xlen) : LinuxSystemEmulationModel(xlen==64)
{

}

gensim::DecodeContext* RiscVSystemEmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu)
{
	return new archsim::arch::riscv::RiscVDecodeContext(cpu.GetArch());
}

bool RiscVSystemEmulationModel::Initialise(System& system, archsim::uarch::uArch& uarch)
{
	if (!SystemEmulationModel::Initialise(system, uarch))
		return false;

	// Initialise "physical memory".
	if (GetMemoryModel().GetMappingManager())
		GetMemoryModel().GetMappingManager()->MapRegion(Address(0), 0x40000000, archsim::abi::memory::RegFlagReadWriteExecute, "memory");

	return true;
}


void RiscVSystemEmulationModel::DestroyDevices()
{

}

ExceptionAction RiscVSystemEmulationModel::HandleException(archsim::core::thread::ThreadInstance* cpu, uint32_t category, uint32_t data)
{
	UNIMPLEMENTED;
}

bool RiscVSystemEmulationModel::InstallDevices()
{
	return InstallCoreDevices() && InstallPlatformDevices();
}

bool RiscVSystemEmulationModel::InstallCoreDevices()
{
	main_thread_->GetPeripherals().RegisterDevice("mmu", new archsim::arch::riscv::RiscVMMU());

	return true;
}

bool RiscVSystemEmulationModel::InstallPlatformDevices()
{
	using namespace archsim::module;

	auto &mm = GetSystem().GetModuleManager();
	auto adm = mm.GetModule("devices.arm.system");

	// what platform do we actually have??? how does the kernel tell?

	// PLIC

	// let's put in a pl011
	auto pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x10009000));
//	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(44));
	pl011->SetParameter("SerialPort", new abi::devices::ConsoleSerialPort());
	RegisterMemoryComponent(*pl011);

	return true;
}

bool RiscVSystemEmulationModel::InstallPlatform(loader::BinaryLoader& loader)
{
//	if (!LinuxSystemEmulationModel::InstallPlatform(loader))
//		return false;

	return InstallBootloader(loader);
}

bool RiscVSystemEmulationModel::InstallBootloader(loader::BinaryLoader &loader)
{
	return true;
}

bool RiscVSystemEmulationModel::PrepareCore(archsim::core::thread::ThreadInstance& core)
{
	// invoke reset exception
	core.GetArch().GetISA("riscv").GetBehaviours().GetBehaviour("riscv_reset").Invoke(&core, {});

	// fill in register values


	return true;
}


RegisterComponent(archsim::abi::EmulationModel, RVSystemEmulationModel32, "riscv32-system", "RISC-V System emulation model");
RegisterComponent(archsim::abi::EmulationModel, RVSystemEmulationModel64, "riscv64-system", "RISC-V System emulation model");
