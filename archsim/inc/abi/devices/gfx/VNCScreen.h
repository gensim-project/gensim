/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   VNCScreen.h
 * Author: harry
 *
 * Created on 08 May 2018, 16:50
 */

#ifndef VNCSCREEN_H
#define VNCSCREEN_H

#include "abi/devices/gfx/VirtualScreen.h"

#include <libgvnc/Server.h>
#include <libgvnc/Framebuffer.h>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace gfx
			{
				class VNCScreen : public VirtualScreen
				{
				public:
					VNCScreen(std::string id, memory::MemoryModel *mem_model, System* sys);

					bool Initialise() override;
					bool Reset() override;
					void SetKeyboard(generic::Keyboard& kbd) override;
					void SetMouse(generic::Mouse& mouse) override;
					bool Configure(uint32_t width, uint32_t height, VirtualScreenMode mode) override;
					bool SetFramebufferPointer(Address guest_addr) override;

				private:
					void *fb_ptr_;

					libgvnc::Server *vnc_server_;

					libgvnc::Framebuffer *vnc_framebuffer_;

					generic::Keyboard *keyboard_;
					generic::Mouse *mouse_;
				};
			}
		}
	}
}

#endif /* VNCSCREEN_H */

