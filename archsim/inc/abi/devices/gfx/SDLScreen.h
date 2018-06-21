/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * SDLScreen.h
 *
 *  Created on: 22 Jul 2014
 *      Author: harry
 */

#ifndef SDLSCREEN_H_
#define SDLSCREEN_H_

#include "abi/devices/SerialPort.h"
#include "abi/devices/gfx/VirtualScreen.h"
#include "abi/devices/gfx/VirtualScreenManager.h"
#include "concurrent/Thread.h"
#include "abi/memory/MemoryModel.h"

#include <SDL2/SDL.h>
#include <mutex>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				class Keyboard;
				class Mouse;
			}
			namespace gfx
			{


				class SDLScreen : public VirtualScreen, private concurrent::Thread
				{
				public:
					SDLScreen(std::string id, memory::MemoryModel *mem_model, System* sys);
					~SDLScreen();

					bool Initialise() override;
					bool Reset() override;

					void SetKeyboard(generic::Keyboard& kbd) override
					{
						this->kbd = &kbd;
					}
					void SetMouse(generic::Mouse& mouse) override
					{
						this->mouse = &mouse;
					}

				private:
					void run() override;

					void terminate();

					void draw_frame();

					void PerformKeyboardEvent(bool press, SDL_Scancode scancode);

					bool draw_rgb();
					bool draw_doom();

					volatile bool terminated;
					volatile bool running;

					SDL_Window *window;
					SDL_Renderer *renderer;
					SDL_Texture *window_texture;

					generic::Keyboard *kbd;
					generic::Mouse *mouse;

					System *system;

					bool hw_accelerated;

					static std::mutex _sdl_lock;
					static bool _sdl_initialised;
				};

				extern VirtualScreenManager<SDLScreen> SDLScreenManager;

			}
		}
	}
}


#endif /* SDLSCREEN_H_ */
