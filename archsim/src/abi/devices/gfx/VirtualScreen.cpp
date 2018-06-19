/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/gfx/VirtualScreen.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"

UseLogContext(LogGfx);
DeclareChildLogContext(LogVirtualScreen, LogGfx, "VS");

using namespace archsim::abi::devices::gfx;

static archsim::abi::devices::ComponentDescriptor vs_descriptor ("VirtualScreen");
VirtualScreen::VirtualScreen(std::string id, memory::MemoryModel *mem) : Component(vs_descriptor), fb_ptr(0), p_ptr(0), id(id), memory(mem), initialised(false), width(0), height(0), mode(VSM_None)
{

}

VirtualScreen::~VirtualScreen()
{

}

bool VirtualScreen::Configure(uint32_t width, uint32_t height, VirtualScreenMode mode)
{
	this->width = width;
	this->height = height;
	this->mode = mode;

	return true;
}

bool VirtualScreen::SetFramebufferPointer(uint32_t guest_addr)
{
	fb_ptr = guest_addr;
	return true;
}

bool VirtualScreen::SetPalettePointer(uint32_t guest_addr)
{
	p_ptr = guest_addr;
	return true;
}

bool VirtualScreen::SetPaletteMode(PaletteMode new_mode)
{
	if(new_mode == palette_mode) return true;

	palette_mode = new_mode;

	// If we're switching to an in-memory palette, get rid of the locally held palette
	if(new_mode == PaletteInGuestMemory) free(palette_entries);

	return true;
}

bool VirtualScreen::SetPaletteSize(uint32_t new_size)
{
	assert(palette_mode == PaletteDirect);

	palette_size = new_size;
	palette_entries = (uint32_t*)realloc(palette_entries, sizeof(uint32_t*) * new_size);

	return true;
}

bool VirtualScreen::SetPaletteEntry(uint32_t index, uint32_t data)
{
	assert(palette_mode == PaletteDirect);
	assert(palette_entries);

	palette_entries[index] = data;

	return true;
}

NullScreen::NullScreen(std::string id, memory::MemoryModel *mem_model, System *) : VirtualScreen(id, mem_model) {}
NullScreen::~NullScreen() {}
bool NullScreen::Initialise()
{
	return true;
}
bool NullScreen::Reset()
{
	return true;
}

void NullScreen::SetKeyboard(generic::Keyboard&) {}
void NullScreen::SetMouse(generic::Mouse&) {}

class NullScreenManager : public VirtualScreenManager<NullScreen> {};

RegisterComponent(VirtualScreenManagerBase, NullScreenManager, "Null", "Null screen - no output");
