/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/gfx/FBScreen.h"
#include "util/ComponentManager.h"
#include "abi/memory/MemoryModel.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace archsim::abi::devices::gfx;

FBScreen::FBScreen(std::string id, memory::MemoryModel* mem_model, System* sys) : VirtualScreen(id, mem_model)
{

}

FBScreen::~FBScreen()
{
	if(tty_) {
//		ioctl(fileno(tty_), KDSETMODE, KD_TEXT);
	}
	fclose(tty_);
	fclose(fb_);
}

bool FBScreen::Initialise()
{
	tty_ = fopen("/dev/tty", "wr");
	if(tty_ < 0) {
		return false;
	}

//	if(ioctl(fileno(tty_), KDSETMODE, KD_GRAPHICS)) {
//		return false;
//	}

	fb_ = fopen("/dev/fb0", "wr");
	if(fb_ < 0) {
		return false;
	}

	if(ioctl(fileno(fb_), FBIOGET_FSCREENINFO, &f_info_) != -1) {
		return false;
	}
	if(ioctl(fileno(fb_), FBIOGET_VSCREENINFO, &v_info_) != -1) {
		return false;
	}

	uint32_t fb_size = f_info_.line_length * v_info_.yres;
	fb_buffer_ = mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fb_), 0);

	start();

	return true;
}

bool FBScreen::Reset()
{

}

void FBScreen::run()
{
	running_ = true;
	while(running_) {
		// copy target framebuffer to host framebuffer
		GetMemory()->Read(fb_ptr.Get(), (uint8_t*)fb_buffer_, GetWidth() * GetHeight());
		usleep(100000);
	}
}


void FBScreen::SetKeyboard(generic::Keyboard& kbd)
{

}

void FBScreen::SetMouse(generic::Mouse& mouse)
{

}

class FBScreenManager : public VirtualScreenManager<FBScreen> {};
RegisterComponent(VirtualScreenManagerBase, FBScreenManager, "FB", "Framebuffer based screen manager")

