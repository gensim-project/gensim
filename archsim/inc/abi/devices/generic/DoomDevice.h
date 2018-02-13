/*
 * File:   DoomKeyboard.h
 * Author: s0457958
 *
 * Created on 24 September 2014, 16:30
 */

#ifndef DOOMKEYBOARD_H
#define	DOOMKEYBOARD_H

#include "abi/devices/generic/Keyboard.h"
#include "abi/devices/arm/core/ArmCoprocessor.h"
#include "abi/devices/gfx/VirtualScreenManager.h"

#include <chrono>
#include <list>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace gfx
			{
				class VirtualScreen;
			}

			namespace generic
			{
				class DoomDevice : public ArmCoprocessor, public Keyboard
				{
				public:
					DoomDevice(gfx::VirtualScreenManagerBase& screen);
					virtual ~DoomDevice();

					virtual bool access_cp0(bool is_read, uint32_t& data) override;
					virtual bool access_cp1(bool is_read, uint32_t& data) override;
					virtual bool access_cp2(bool is_read, uint32_t& data) override;
					virtual bool access_cp3(bool is_read, uint32_t& data) override;
					virtual bool access_cp10(bool is_read, uint32_t& data) override;

					bool Initialise() override;

					void KeyDown(uint32_t keycode) override;
					void KeyUp(uint32_t keycode) override;

				private:
					std::chrono::high_resolution_clock::time_point tp;
					gfx::VirtualScreenManagerBase& screenMan;
					std::list<uint32_t> buffer;

					bool screen_online;

					uint32_t target_width, target_height;
				};
			}
		}
	}
}

#endif	/* DOOMKEYBOARD_H */

