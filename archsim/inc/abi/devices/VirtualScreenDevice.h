/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   VirtualScreenDevice.h
 * Author: s0457958
 *
 * Created on 30 January 2014, 12:39
 */

#ifndef VIRTUALSCREENDEVICE_H
#define	VIRTUALSCREENDEVICE_H

#include "abi/devices/Component.h"
#include "abi/devices/Device.h"

#include "concurrent/Mutex.h"
#include "concurrent/Thread.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Surface;

#include <queue>

#define SCREEN_XDIM 160
#define SCREEN_YDIM 128

namespace archsim
{
	namespace abi
	{
		class EmulationModel;

		namespace devices
		{
			static const uint32_t kBaseAddress = 0xff010000;
			static const uint32_t kPalettePointerAddr = 0x10000 - 24;
			static const uint32_t kFramebufferWidthAddr = 0x10000 - 16;
			static const uint32_t kFramebufferHeightAddr = 0x10000 - 12;
			static const uint32_t kFramebufferModeAddr = 0x10000 - 8;
			static const uint32_t kFramebufferPointerAddr = 0x10000 - 4;

			class VirtualScreenDevice;

			class VirtualScreenDeviceKeyboardController : public ArmCoprocessor
			{
			public:
				VirtualScreenDeviceKeyboardController(VirtualScreenDevice &screen);
				virtual bool access_cp0(bool is_read, uint32_t &data);

			private:
				VirtualScreenDevice &screen;
			};

			class VirtualScreenDevice : public SystemComponent, public archsim::concurrent::Thread
			{
			public:
				VirtualScreenDevice(EmulationModel& model);
				~VirtualScreenDevice();

				int GetComponentID();

				bool Attach();

				inline uint32_t pop_kb_scancode()
				{
					uint32_t sc = kb_fifo.back();
					kb_fifo.pop();
					return sc;
				}
				inline uint32_t kb_queue_depth() const
				{
					return kb_fifo.size();
				}

			private:
				void run();
				void init_window();
				void draw();

//	bool on_configure_event(GdkEventConfigure* event);
//	bool on_keyboard_event(GdkEvent* event);

				uint32_t get_scancode(uint32_t keyval, bool press);

				void frame_buffer_parse();
				void frame_buffer_draw();

				void ParseYUV();
				void ParseRGB();
				void ParseDoom();

				EmulationModel& emulation_model;

				SDL_Window *window;
				SDL_Renderer *renderer;
				SDL_Surface *window_surface;
				SDL_Texture *window_texture;

				uint32_t fb_mode, fb_width, fb_height, fb_addr, fb_old_size, pb_addr;
				uint8_t *framebuffer;

				uint8_t *temp_framebuffer, *temp_palbuffer;

				volatile bool terminate;

				std::queue<uint32_t> kb_fifo;
			};
		}
	}
}

#endif	/* VIRTUALSCREENDEVICE_H */

