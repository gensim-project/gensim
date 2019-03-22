/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   FBScreen.h
 * Author: harry
 *
 * Created on 26 July 2018, 11:55
 */

#ifndef FBSCREEN_H
#define FBSCREEN_H

#include "abi/devices/gfx/VirtualScreen.h"
#include "concurrent/Thread.h"

#include <linux/fb.h>
#include <linux/kd.h>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace gfx
			{

				class FBScreen : public archsim::abi::devices::gfx::VirtualScreen, concurrent::Thread
				{
				public:
					FBScreen(std::string id, memory::MemoryModel *mem_model, System* sys);
					virtual ~FBScreen();

					bool Initialise() override;
					bool Reset() override;
					void SetKeyboard(generic::Keyboard& kbd) override;
					void SetMouse(generic::Mouse& mouse) override;

				private:
					void run() override;

					bool running_;
					FILE *tty_, *fb_;
					struct fb_var_screeninfo v_info_;
					struct fb_fix_screeninfo f_info_;
					void *fb_buffer_;

				};

			}
		}
	}
}

#endif /* FBSCREEN_H */

