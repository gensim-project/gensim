/*
 * ElfSystemEmulationModel.cpp
 *
 *  Created on: 8 Sep 2014
 *      Author: harry
 */

#include "arch/arm/AngelSyscallHandler.h"

#include "abi/ElfSystemEmulationModel.h"
#include "abi/loader/BinaryLoader.h"

#include "gensim/gensim_processor.h"

#include "util/ComponentManager.h"
#include "util/SimOptions.h"
#include "util/LogContext.h"

#define STACK_SIZE	0x40000

UseLogContext(LogSystemEmulationModel);
DeclareChildLogContext(LogElfSystemEmulationModel, LogSystemEmulationModel, "ELF");

using namespace archsim::abi;

//RegisterComponent(EmulationModel, ElfSystemEmulationModel, "elf-system", "An emulation model for running ELF binaries in a bare metal context")

ElfSystemEmulationModel::ElfSystemEmulationModel() : initial_sp(0)
{
	heap_base = 0x20000000;
	heap_limit = 0x30000000;
	stack_base = 0x50000000;
	stack_limit = 0x40000000;
}

ElfSystemEmulationModel::~ElfSystemEmulationModel()
{

}

bool ElfSystemEmulationModel::Initialise(System &system, archsim::uarch::uArch &uarch)
{
	bool rc = SystemEmulationModel::Initialise(system, uarch);
	if (!rc)
		return false;

	// Initialise "physical memory".
	GetMemoryModel().GetMappingManager()->MapRegion(0, 0xffff0000, (archsim::abi::memory::RegionFlags)7, "phys-mem");

	return true;
}

void ElfSystemEmulationModel::Destroy()
{
	SystemEmulationModel::Destroy();
}

ExceptionAction ElfSystemEmulationModel::HandleException(archsim::ThreadInstance *cpu, uint32_t category, uint32_t data)
{
	if(category == 3 && data == 0x123456) {
		if(HandleSemihostingCall()) {
			return ResumeNext;
		} else {
			// This is just a debug message as the semihosting interface should produce
			// a meaningful LC_ERROR message if there was an actual error
			LC_DEBUG1(LogElfSystemEmulationModel) << "A semihosting call caused a simulation abort.";
			return AbortSimulation;
		}
	}
	//We should not take any exceptions in this context
	LC_ERROR(LogElfSystemEmulationModel) << "An exception was generated so aborting simulation. Category " << category << ", Data " << data;
	return AbortSimulation;
}

void ElfSystemEmulationModel::DestroyDevices()
{

}

bool ElfSystemEmulationModel::InstallPlatform(loader::BinaryLoader& loader)
{
	binary_entrypoint = loader.GetEntryPoint();

	if (!archsim::options::Bootloader.IsSpecified()) {
		LC_ERROR(LogElfSystemEmulationModel) << "Bootloader must be specified for ELF system emulation.";
		return false;
	}

	if (!InstallBootloader(archsim::options::Bootloader)) {
		LC_ERROR(LogElfSystemEmulationModel) << "Bootloader installation failed";
		return false;
	}

	if (!InstallKernelHelpers()) {
		LC_ERROR(LogElfSystemEmulationModel) << "Kernel helper installation failed";
		return false;
	}

	if (!PrepareStack()) {
		LC_ERROR(LogElfSystemEmulationModel) << "Error initialising stack";
		return false;
	}

	return true;
}

bool ElfSystemEmulationModel::PrepareCore(gensim::Processor &cpu)
{
	LC_DEBUG1(LogElfSystemEmulationModel) << "Binary entry point: " << std::hex << binary_entrypoint;

	// Load r12 with the entry-point to the binary being executed.
	uint32_t *regs = (uint32_t *)cpu.GetRegisterBankDescriptor("RB").GetBankDataStart();
	regs[12] = binary_entrypoint;

	LC_DEBUG1(LogElfSystemEmulationModel) << "Initial SP: " << std::hex << initial_sp;

	// Load sp with the top of the stack.
	regs = (uint32_t *)cpu.GetRegisterBankDescriptor("RB").GetBankDataStart();
	regs[13] = initial_sp;

	return true;
}

bool ElfSystemEmulationModel::HandleSemihostingCall()
{
	archsim::arch::arm::AngelSyscallHandler angel (GetBootCore()->GetMemoryModel(), heap_base, heap_limit, stack_base, stack_limit);
	return angel.HandleSyscall(*GetBootCore());
}

bool ElfSystemEmulationModel::InstallDevices()
{
	archsim::abi::devices::Device *coprocessor;
	if (!GetComponentInstance("armcoprocessor", coprocessor)) {
		LC_ERROR(LogElfSystemEmulationModel) << "Unable to instantiate ARM coprocessor";
		return false;
	}

	cpu->peripherals.RegisterDevice("coprocessor", coprocessor);
	cpu->peripherals.AttachDevice("coprocessor", 15);

	archsim::abi::devices::Device *mmu;
	if (!GetComponentInstance("ARM926EJSMMU", mmu)) {
		LC_ERROR(LogElfSystemEmulationModel) << "Unable to instantiate MMU";
		return false;
	}

	cpu->peripherals.RegisterDevice("mmu", mmu);
	cpu->peripherals.InitialiseDevices();

	return true;
}

bool ElfSystemEmulationModel::InstallBootloader(std::string filename)
{
	loader::SystemElfBinaryLoader loader(*this, true);

	LC_DEBUG1(LogElfSystemEmulationModel) << "Installing Bootloader from '"	<< filename << "'";

	if (!loader.LoadBinary(filename)) {
		LC_ERROR(LogElfSystemEmulationModel) << "Bootloader loading failed";
		return false;
	}

	return true;
}

bool ElfSystemEmulationModel::PrepareStack()
{
	initial_sp = 0xc0000000 + STACK_SIZE;

	return true;
}

bool ElfSystemEmulationModel::InstallKernelHelpers()
{
	LC_DEBUG1(LogElfSystemEmulationModel) << "Initialising ARM Kernel Helpers";

	memory::guest_addr_t kernel_helper_region = 0xffff0000;
	GetMemoryModel().GetMappingManager()->MapRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead), "[eabi]");

	return true;
}
