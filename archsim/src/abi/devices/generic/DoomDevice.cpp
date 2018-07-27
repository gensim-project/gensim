/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/DoomDevice.h"
#include "abi/devices/PeripheralManager.h"
#include "abi/EmulationModel.h"
#include "abi/devices/gfx/VirtualScreen.h"

#include <chrono>

using namespace archsim::abi::devices::generic;

DoomDevice::DoomDevice(devices::gfx::VirtualScreenManagerBase& screen) : screenMan(screen), screen_online(false)
{

}

DoomDevice::~DoomDevice()
{

}

bool DoomDevice::Initialise()
{
	return true;
}


void DoomDevice::KeyDown(uint32_t keycode)
{
	buffer.push_back(keycode);
}

void DoomDevice::KeyUp(uint32_t keycode)
{
	buffer.push_back(keycode | 0xffff0000);
}

bool DoomDevice::access_cp0(bool is_read, uint32_t& data)
{
	assert(is_read);

	data = buffer.size();
	return true;
}

bool DoomDevice::access_cp1(bool is_read, uint32_t& data)
{
	assert(is_read);

	if (buffer.size() == 0) {
		data = 0;
	} else {
		data = buffer.front();
		buffer.pop_front();
	}

	return true;
}


bool DoomDevice::access_cp3(bool is_read, uint32_t& data)
{
	assert(!is_read);

	if(opc1 == 0) target_width = data;
	if(opc1 == 1) target_height = data;
	if(opc1 == 2) {
		fprintf(stderr, "Bringing screen up\n");

		auto &screen = *screenMan.CreateScreenInstance("Display", &Manager->GetEmulationModel()->GetMemoryModel(), &Manager->GetEmulationModel()->GetSystem());
		screen.SetKeyboard(*this);
		screen.SetFramebufferPointer(Address(data));
		screen.Configure(target_width, target_height, devices::gfx::VSM_RGBA8888);
		screen.Initialise();
	}

	return true;
}

bool DoomDevice::access_cp2(bool is_read, uint32_t& data)
{
	assert(!is_read);

	if (!screen_online) {
//		screen_online = true;
//		screen.SetKeyboard(*this);
//		screen.SetPalettePointer(data);
//		screen.Configure(320, 200, devices::gfx::VSM_DoomPalette);
//		screen.Initialise();
	}

	return true;
}

bool DoomDevice::access_cp10(bool is_read, uint32_t& data)
{
	assert(is_read);

	if (opc1 == 0) {
		tp = std::chrono::high_resolution_clock::now();
		data = 0;
		return true;
	} else if (opc1 == 1) {
		data = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tp).count();
		return true;
	}

	return false;
}
