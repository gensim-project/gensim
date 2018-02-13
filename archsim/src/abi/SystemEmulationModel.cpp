/*
 * abi/SystemEmulationModel.cpp
 */
#include "abi/SystemEmulationModel.h"
#include "abi/loader/BinaryLoader.h"
#include "abi/devices/Device.h"
#include "abi/devices/DeviceManager.h"
#include "abi/devices/Component.h"
#include "abi/devices/MMU.h"
#include "abi/memory/MemoryModel.h"

#include "abi/memory/system/FunctionBasedSystemMemoryModel.h"

#include "abi/memory/system/CacheBasedSystemMemoryModel.h"

#include "gensim/gensim.h"
#include "gensim/gensim_processor.h"

#include "util/ComponentManager.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

using namespace archsim::abi;
using namespace archsim::abi::memory;

UseLogContext(LogEmulationModel);
DeclareChildLogContext(LogSystemEmulationModel, LogEmulationModel, "System");

SystemEmulationModel::SystemEmulationModel()
{
}

SystemEmulationModel::~SystemEmulationModel() { }

void SystemEmulationModel::PrintStatistics(std::ostream& stream)
{
	cpu->PrintStatistics(stream);
}

bool SystemEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	// Initialise the underlying emulation model
	if (!EmulationModel::Initialise(system, uarch))
		return false;

	// Acquire the CPU component
	auto moduleentry = GetSystem().GetModuleManager().GetModule(archsim::options::ProcessorName)->GetEntry<archsim::module::ModuleProcessorEntry>("CPU");
	cpu = moduleentry->Get(archsim::options::ProcessorName, 0, &GetSystem().GetPubSub());


	// Create a system memory model for this CPU
	SystemMemoryModel *smm = NULL;
	if(!GetComponentInstance(archsim::options::SystemMemoryModel, smm, &GetMemoryModel(), &system.GetPubSub())) {
		LC_ERROR(LogSystemEmulationModel) << "Unable to acquire system memory model";
		LC_ERROR(LogSystemEmulationModel) << GetRegisteredComponents(smm, &GetMemoryModel(), &system.GetPubSub());
		return false;
	}

	if (!cpu->Initialise(*this, *smm)) {
		delete smm;
		return false;
	}

	// Initialise the CPU
	cpu->reset_to_initial_state(true);

	// Install Devices
	if (!InstallDevices()) {
		return false;
	}

	// Obtain the MMU
	devices::MMU *mmu = (devices::MMU*)cpu->peripherals.GetDeviceByName("mmu");

	// Update the memory model with the necessary object references
	smm->SetMMU(mmu);
	smm->SetCPU(cpu);
	smm->SetDeviceManager(&base_device_manager);

	mmu->SetPhysMem(&GetMemoryModel());

	// Initialise the memory model
	if (!smm->Initialise()) {
		return false;
	}

	return true;
}

void SystemEmulationModel::Destroy()
{
	DestroyDevices();

	cpu->GetMemoryModel().Destroy();
	delete &cpu->GetMemoryModel();

	cpu->Destroy();
	delete cpu;
}

gensim::Processor *SystemEmulationModel::GetCore(int id)
{
	if (id != 0) return NULL;

	return cpu;
}

gensim::Processor *SystemEmulationModel::GetBootCore()
{
	return cpu;
}

void SystemEmulationModel::ResetCores()
{
	cpu->reset_exception();
}

void SystemEmulationModel::HaltCores()
{
	cpu->Halt();
}

bool SystemEmulationModel::PrepareBoot(System &system)
{
	loader::BinaryLoader *loader;
	if (archsim::options::TargetBinaryFormat == "elf") {
		loader = new loader::SystemElfBinaryLoader(*this, archsim::options::TraceSymbols);
	} else if (archsim::options::TargetBinaryFormat == "zimage") {
		if (!archsim::options::ZImageSymbolMap.IsSpecified()) {
			loader = new loader::ZImageBinaryLoader(*this, 64 * 1024);
		} else {
			loader = new loader::ZImageBinaryLoader(*this, 64 * 1024, archsim::options::ZImageSymbolMap);
		}
	} else {
		LC_ERROR(LogSystemEmulationModel) << "Unknown binary format: " << archsim::options::TargetBinaryFormat.GetValue();
		return false;
	}

	// Load the binary.
	if (!loader->LoadBinary(archsim::options::TargetBinary)) {
		delete loader;
		LC_ERROR(LogSystemEmulationModel) << "Unable to load binary: " << (std::string)archsim::options::TargetBinary;
		return false;
	}

	// Run platform-specific initialisation
	if (!InstallPlatform(*loader)) {
		LC_ERROR(LogSystemEmulationModel) << "Unable to install platform";
		return false;
	}

	delete loader;

	// Run platform-specific CPU boot code
	if (!PrepareCore(*cpu)) {
		LC_ERROR(LogSystemEmulationModel) << "Unable to prepare CPU for booting";
		return false;
	}

	// Take a reset exception.
	cpu->reset_exception();
	return true;
}

bool SystemEmulationModel::RegisterMemoryComponent(abi::devices::MemoryComponent& component)
{
	LC_INFO(LogSystemEmulationModel) << "Registering device at " << component.GetBaseAddress() << " to " << Address(component.GetBaseAddress().Get() + component.GetSize());
	return base_device_manager.InstallDevice(component.GetBaseAddress().Get(), component.GetSize(), component);
}

void SystemEmulationModel::RegisterCoreComponent(abi::devices::CoreComponent& component)
{

}
