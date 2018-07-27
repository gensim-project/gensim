/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/x86/X86DoomLinuxUserEmulationModel.h"
#include "abi/devices/gfx/VirtualScreen.h"
#include "util/ComponentManager.h"

using namespace archsim::arch::x86;

RegisterComponent(archsim::abi::EmulationModel, X86DoomLinuxUserEmulationModel, "x86-user-doom", "ARM Linux user emulation model");

bool X86DoomLinuxUserEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if(!X86LinuxUserEmulationModel::Initialise(system, uarch)) {
		return false;
	}

	archsim::abi::devices::gfx::VirtualScreenManagerBase *screen_man = NULL;
	if(!GetComponentInstance(archsim::options::ScreenManagerType, screen_man)) {
		fprintf(stderr, "Could not instantiate Screen Manager %s!\n%s\n", archsim::options::ScreenManagerType.GetValue().c_str(), GetRegisteredComponents(screen_man).c_str());;
		return false;
	}

	auto screen = screen_man->CreateScreenInstance("lcd", &GetMemoryModel(), &GetSystem());

	GetMemoryModel().GetMappingManager()->MapRegion(Address(0xe0000000), 640 * 480 * 3, archsim::abi::memory::RegFlagReadWrite, "Screen");
	screen->SetFramebufferPointer(Address(0xe0000000));
	screen->Configure(640, 480, archsim::abi::devices::gfx::VirtualScreenMode::VSM_RGB);
	screen->Initialise();

	return true;
}
