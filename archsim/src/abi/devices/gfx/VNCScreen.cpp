/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "abi/devices/gfx/VNCScreen.h"
#include "util/ComponentManager.h"
#include "abi/memory/MemoryModel.h"

#include "abi/devices/generic/Keyboard.h"
#include "abi/devices/generic/Mouse.h"

using namespace archsim::abi::devices::gfx;

VNCScreen::VNCScreen(std::string id, memory::MemoryModel* mem_model, System* sys) : VirtualScreen(id, mem_model)
{

}

libgvnc::FB_PixelFormat Format_16bit (16, 16, 0, 1, 31, 63, 31, 11, 5, 0);
libgvnc::FB_PixelFormat Format_RGB32 (32, 24, 0, 1, 255, 255, 255, 16, 8, 0);


static uint32_t MapSym(uint32_t xsym) {
	switch(xsym) {
		case ' ': return 0x29;
		
		case 'a': return 0x1c;
		case 'A': return 0x1c;
		case 'b': return 0x32;
		case 'B': return 0x32;
		case 'c': return 0x21;
		case 'C': return 0x21;
		case 'd': return 0x23;
		case 'D': return 0x23;
		case 'e': return 0x24;
		case 'E': return 0x24;
		case 'f': return 0x2B;
		case 'F': return 0x2B;
		case 'g': return 0x34;
		case 'G': return 0x34;
		case 'h': return 0x33;
		case 'H': return 0x33;
		case 'i': return 0x43;
		case 'I': return 0x43;
		case 'j': return 0x3B;
		case 'J': return 0x3B;
		case 'k': return 0x42;
		case 'K': return 0x42;
		case 'l': return 0x4B;
		case 'L': return 0x4B;
		case 'm': return 0x3a;
		case 'M': return 0x3a;
		case 'n': return 0x31;
		case 'N': return 0x31;
		case 'o': return 0x44;
		case 'O': return 0x44;
		case 'p': return 0x4D;
		case 'P': return 0x4D;
		case 'q': return 0x15;
		case 'Q': return 0x15;
		case 'r': return 0x2d;
		case 'R': return 0x2d;
		case 's': return 0x1b;
		case 'S': return 0x1b;
		case 't': return 0x2c;
		case 'T': return 0x2c;
		case 'u': return 0x3c;
		case 'U': return 0x3c;
		case 'v': return 0x2A;
		case 'V': return 0x2A;
		case 'w': return 0x1d;
		case 'W': return 0x1d;
		case 'x': return 0x22;
		case 'X': return 0x22;
		case 'y': return 0x35;
		case 'Y': return 0x35;
		case 'z': return 0x1a;
		case 'Z': return 0x1a;
		
		case '0': return 0x45;
		case '1': return 0x16;
		case '2': return 0x1E;
		case '3': return 0x26;
		case '4': return 0x25;
		case '5': return 0x2E;
		case '6': return 0x36;
		case '7': return 0x3D;
		case '8': return 0x3E;
		case '9': return 0x46;
		
		case '`': return 0; //todo
//		case '¬': return 0; //todo almagni: this character cannot be encoded in 1 byte.
		case '!': return 0; //todo
		case '"': return 0; //todo
//		case '£': return 0; //todo almagni: this character cannot be encoded in 1 byte.  
		case '$': return 0; //todo
		case '%': return 0; //todo
		case '^': return 0; //todo
		case '&': return 0; //todo
		case '*': return 0; //todo
		case '(': return 0; //todo
		case ')': return 0; //todo
		case '-': return 0x4e;
		case '_': return 0x4e; //todo
		case '=': return 0x55;
		case '+': return 0; //todo
		case '[': return 0x1a; //todo
		case ']': return 0x1b; //todo
		case '{': return 0; //todo
		case '}': return 0; //todo
		case ';': return 0x4c;
		case ':': return 0x4c; //todo
		case '\'': return 0x52;
		case '@': return 0; //todo
		case '#': return 0; //todo
		case '~': return 0x0e;
		case '|': return 0x5d;
		case '\\': return 0x5d;
		case ',': return 0x41;
		case '<': return 0x41; //todo
		case '.': return 0x49;
		case '>': return 0x49; //todo
		case '/': return 0x4a;
		case '?': return 0x4a; //todo
		
		case 0xff08: return 0x66; // backspace
		case 0xff09: return 0x0d; // tab
		case 0xff0d: return 0x5a; // return
		case 0xff1b: return 0x76; // escape
		case 0xff63: return 0xe070; // insert
		case 0xffff: return 0xe071; // delete
		case 0xff50: return 0xe06c; // home
		case 0xff57: return 0xe069; // end
		case 0xff55: return 0xe07d; // page up
		case 0xff56: return 0xe07a; // page down
		
		case 0xff51 : return 0xe06b; // left
		case 0xff52 : return 0xe075; // up
		case 0xff53 : return 0xe074; // right
		case 0xff54 : return 0xe072; // down
		
		case 0xffbe: return 0x05; // f1
		case 0xffbf: return 0x06; // f2
		case 0xffc0: return 0x04; // f3
		case 0xffc1: return 0x0c; // f4
		case 0xffc2: return 0x03; // f5
		case 0xffc3: return 0x0b; // f6
		case 0xffc4: return 0x83; // f7
		case 0xffc5: return 0x0a; // f8
		case 0xffc6: return 0x01; // f9
		case 0xffc7: return 0x09; // f10
		case 0xffc8: return 0x78; // f11
		case 0xffc9: return 0x07; // f12
		case 0xffe1: return 0x12; // shift l
		case 0xffe2: return 0x59; // shift r
		case 0xffe3: return 0x14; // ctrl l
		case 0xffe4: return 0xe014; // ctrl r
		case 0xffe9: return 0x11; // alt l
		case 0xffea: return 0xe011; // alt r
		default:
			return xsym;
	}
}

void key_callback(generic::Keyboard &kbd, const libgvnc::KeyEvent &event)
{
	uint32_t key = MapSym(event.KeySym);
	if(event.Down) {
		kbd.KeyDown(key);
	} else {
		kbd.KeyUp(key);
	}
}

void mouse_callback(generic::Mouse &mouse, const libgvnc::PointerEvent &event)
{
	static int prevButtonMask = 0;
	
	mouse.Move(event.PositionX, event.PositionY);
	
	if(event.ButtonMask != prevButtonMask) {
		for(int i = 0; i < 8; ++i) {
			if((event.ButtonMask & (1 << i)) != (prevButtonMask & (1 << i))) {
				if(event.ButtonMask & (1 << i)) {
					mouse.ButtonDown(i);
				} else {
					mouse.ButtonUp(i);
				}
			}
		}
	}
	prevButtonMask = event.ButtonMask;
}

bool VNCScreen::Configure(uint32_t width, uint32_t height, VirtualScreenMode mode)
{
	// figure out pixel format
	libgvnc::FB_PixelFormat pixelformat = Format_16bit;
	
	vnc_framebuffer_ = new libgvnc::Framebuffer(width, height, pixelformat);
	vnc_framebuffer_ ->SetData(fb_ptr_);
	vnc_server_ = new libgvnc::Server(vnc_framebuffer_);
	
	vnc_server_->GetKeyboard().SetCallback([this](const libgvnc::KeyEvent &event){ key_callback(*keyboard_, event); });
	vnc_server_->GetPointer().SetCallback([this](const libgvnc::PointerEvent &event){ mouse_callback(*mouse_, event); });
	
	vnc_server_->Open(5900);
	
	return true;
}


bool VNCScreen::Initialise()
{
	return true;	
}

bool VNCScreen::Reset()
{
	return true;
}

bool VNCScreen::SetFramebufferPointer(uint32_t guest_addr)
{
	host_addr_t addr;
	GetMemory()->LockRegion(guest_addr, 4096, addr);
	fb_ptr_ = addr;
	return true;
}


void VNCScreen::SetKeyboard(generic::Keyboard& kbd)
{
	keyboard_ = &kbd;
}

void VNCScreen::SetMouse(generic::Mouse& mouse)
{
	mouse_ = &mouse;
}

class VNCScreenManager : public VirtualScreenManager<VNCScreen> {};
RegisterComponent(VirtualScreenManagerBase, VNCScreenManager, "VNC", "libgvnc based VNC screen (port 5900).");
