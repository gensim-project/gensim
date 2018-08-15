/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/x86/X86DoomLinuxUserEmulationModel.h"
#include "abi/devices/gfx/VirtualScreen.h"
#include "util/ComponentManager.h"
#include "abi/devices/generic/Keyboard.h"

using namespace archsim::arch::x86;

RegisterComponent(archsim::abi::EmulationModel, X86DoomLinuxUserEmulationModel, "x86-user-doom", "ARM Linux user emulation model");

class DoomKeyboard : public generic::Keyboard
{
public:
	DoomKeyboard(archsim::abi::memory::MemoryModel *mem_model, archsim::Address base_addr) : mem_(mem_model), base_addr_(base_addr) {}
	virtual ~DoomKeyboard()
	{

	}

	void KeyDown(uint32_t keycode) override
	{
		printf("archsim keydown %u\n", keycode);
		mem_->Write32(base_addr_, keycode);
	}
	void KeyUp(uint32_t keycode) override
	{
		// convert keycode to keyup code
		keycode |= 0xf000;
		printf("archsim keyup %u\n", keycode);
		mem_->Write32(base_addr_, keycode);
	}

private:
	archsim::abi::memory::MemoryModel *mem_;
	archsim::Address base_addr_;
};

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
	GetMemoryModel().GetMappingManager()->MapRegion(Address(0xe1000000), 8, archsim::abi::memory::RegFlagReadWrite, "Keyboard");

	DoomKeyboard *dk = new DoomKeyboard(&GetMemoryModel(), Address(0xe1000000));
	screen->SetKeyboard(*dk);

	screen->SetFramebufferPointer(Address(0xe0000000));
	screen->Configure(320, 200, archsim::abi::devices::gfx::VirtualScreenMode::VSM_RGB);
	screen->Initialise();


	return true;
}
