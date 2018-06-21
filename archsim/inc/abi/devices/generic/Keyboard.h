/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Keyboard.h
 * Author: s0457958
 *
 * Created on 02 September 2014, 16:25
 */

#ifndef KEYBOARD_H
#define	KEYBOARD_H

#include "define.h"
#include <vector>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				class Keyboard
				{
				public:
					virtual void KeyDown(uint32_t keycode) = 0;
					virtual void KeyUp(uint32_t keycode) = 0;
				};

				class KeyboardAggregator : public Keyboard
				{
				public:
					inline void AddKeyboard(Keyboard *kbd)
					{
						keyboards.push_back(kbd);
					}

					void KeyDown(uint32_t keycode) override;
					void KeyUp(uint32_t keycode) override;

				private:
					std::vector<Keyboard *> keyboards;
				};
			}
		}
	}
}

#endif	/* KEYBOARD_H */

