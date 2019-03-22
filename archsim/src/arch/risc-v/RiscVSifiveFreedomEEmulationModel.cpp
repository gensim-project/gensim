/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/SerialPort.h"
#include "arch/risc-v/RiscVDecodeContext.h"
#include "arch/risc-v/RiscVMMU.h"
#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "arch/risc-v/RiscVSystemEmulationModel.h"
#include "arch/risc-v/RiscVSifiveFU540EmulationModel.h"
#include "abi/devices/riscv/SifiveCLINT.h"
#include "abi/devices/riscv/SifivePLIC.h"
#include "abi/devices/riscv/SifiveUART.h"
#include "module/ModuleManager.h"
#include "util/ComponentManager.h"
#include "system.h"

using archsim::Address;
using namespace archsim::abi;
using namespace archsim::arch::riscv;

class RiscVSifiveESDKEmulationModel : public RiscVSystemEmulationModel
{
public:
	RiscVSifiveESDKEmulationModel() : RiscVSystemEmulationModel(64) {}
	virtual ~RiscVSifiveESDKEmulationModel() {}


	bool Initialise(System& system, archsim::uarch::uArch& uarch)
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


	void DestroyDevices()
	{

	}

	bool CreateCoreDevices(archsim::core::thread::ThreadInstance* thread) override
	{
		auto mmu = new archsim::arch::riscv::RiscVMMU();
		auto coprocessor = new archsim::arch::riscv::RiscVSystemCoprocessor(thread, mmu);

		thread->GetPeripherals().RegisterDevice("mmu", mmu);
		thread->GetPeripherals().RegisterDevice("coprocessor", coprocessor);

		thread->GetPeripherals().AttachDevice("coprocessor", 0);

		return true;
	}

	bool CreateMemoryDevices() override
	{
		using namespace archsim::module;
		auto hart0 = &GetThread(0);

		// CLINT
		auto clint = new archsim::abi::devices::riscv::SifiveCLINT(*this, Address(0x2000000));
		clint->SetParameter("Hart0", hart0);
		clint->Initialise();
		RegisterMemoryComponent(*clint);

		// PLIC
		auto plic = new archsim::abi::devices::riscv::SifivePLIC(*this, Address(0x0c000000));
		plic->SetParameter("Hart0", hart0);
		RegisterMemoryComponent(*plic);

		auto uart0 = new archsim::abi::devices::riscv::SifiveUART(*this, Address(0x10013000));
		uart0->SetParameter("SerialPort", new archsim::abi::devices::ConsoleOutputSerialPort());
		RegisterMemoryComponent(*uart0);

		auto uart1 = new archsim::abi::devices::riscv::SifiveUART(*this, Address(0x10023000));
		uart1->SetParameter("SerialPort", new archsim::abi::devices::ConsoleOutputSerialPort());
		RegisterMemoryComponent(*uart1);

		return true;
	}

	bool InstallPlatform(loader::BinaryLoader& loader)
	{
		//	if (!LinuxSystemEmulationModel::InstallPlatform(loader))
		//		return false;

		return InstallBootloader(loader);
	}

	bool InstallBootloader(loader::BinaryLoader &loader)
	{
		return true;
	}

	bool PrepareCore(archsim::core::thread::ThreadInstance& core)
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

};

RegisterComponent(archsim::abi::EmulationModel, RiscVSifiveESDKEmulationModel, "riscv64-sifive-ESDK", "SiFive 'E' SDK");
