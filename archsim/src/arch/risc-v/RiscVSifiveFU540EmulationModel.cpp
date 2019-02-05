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

using namespace archsim::abi;
using namespace archsim::arch::riscv;

UseLogContext(LogEmulationModelRiscVSystem)

RiscVSifiveFU540EmulationModel::RiscVSifiveFU540EmulationModel() : RiscVSystemEmulationModel(64)
{

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
	clint->Initialise();
	RegisterMemoryComponent(*clint);

	// PLIC
	auto plic = new archsim::abi::devices::riscv::SifivePLIC(*this, Address(0x0c000000));
	plic->SetParameter("Hart0", main_thread_);
	RegisterMemoryComponent(*plic);

	auto uart0 = new archsim::abi::devices::riscv::SifiveUART(*this, Address(0x10010000));
	uart0->SetParameter("SerialPort", new archsim::abi::devices::ConsoleOutputSerialPort());
	RegisterMemoryComponent(*uart0);

	auto uart1 = new archsim::abi::devices::riscv::SifiveUART(*this, Address(0x10011000));
	uart1->SetParameter("SerialPort", new archsim::abi::devices::ConsoleOutputSerialPort());
	RegisterMemoryComponent(*uart1);

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

RegisterComponent(archsim::abi::EmulationModel, RiscVSifiveFU540EmulationModel, "riscv64-sifive-fu540", "SiFive FU540-C000");
