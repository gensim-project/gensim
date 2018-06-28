/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Mouse.h
 * Author: spink
 *
 * Created on 03 September 2014, 08:07
 */

#ifndef MOUSE_H
#define	MOUSE_H

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
				class Mouse
				{
				public:
					virtual void ButtonDown(int button_index) = 0;
					virtual void ButtonUp(int button_index) = 0;
					virtual void Move(int x, int y) = 0;
				};

				class MouseAggregator : public Mouse
				{
				public:
					inline void AddMouse(Mouse *mouse)
					{
						mice.push_back(mouse);
					}

					void ButtonDown(int button_index) override;
					void ButtonUp(int button_index) override;
					void Move(int x, int y) override;

				private:
					std::vector<Mouse *> mice;
				};
			}
		}
	}
}

#endif	/* MOUSE_H */

