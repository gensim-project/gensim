/*
 * File:   PS2Device.h
 * Author: s0457958
 *
 * Created on 02 September 2014, 15:04
 */

#ifndef PS2DEVICE_H
#define	PS2DEVICE_H

#include "define.h"
#include "abi/devices/generic/Keyboard.h"
#include "abi/devices/generic/Mouse.h"
#include "abi/devices/IRQController.h"
#include "abi/devices/Component.h"

#include <queue>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				namespace ps2
				{
					class PS2Device : public Component
					{
					public:
						PS2Device(IRQLine& irq);

						inline uint32_t Read() const
						{
							uint32_t v = data_queue.front();
							data_queue.pop();
							if (data_queue.empty() && irq.IsAsserted())
								irq.Rescind();
							return v;
						}

						inline bool DataPending() const
						{
							return data_queue.size() > 0;
						}

						virtual void SendCommand(uint32_t command) = 0;

						inline void EnableIRQ()
						{
							irq_enabled = true;
						}
						inline void DisableIRQ()
						{
							irq_enabled = false;
						}
						
						bool Initialise() override;

					protected:
						void QueueData(uint32_t data)
						{
							data_queue.push(data);
							if (irq_enabled && !irq.IsAsserted()) irq.Assert();
						}

					private:
						IRQLine& irq;
						bool irq_enabled;
						mutable std::queue<uint32_t> data_queue;
					};

					class PS2KeyboardDevice : public PS2Device, public Keyboard
					{
					public:
						PS2KeyboardDevice(IRQLine& irq);

						void SendCommand(uint32_t command) override;

						void KeyDown(uint32_t keycode) override;
						void KeyUp(uint32_t keycode) override;

					private:
						uint32_t last_command;
					};

					class PS2MouseDevice : public PS2Device, public Mouse
					{
					public:
						PS2MouseDevice(IRQLine& irq);
						void SendCommand(uint32_t command) override;

						void ButtonDown(int button_index) override;
						void ButtonUp(int button_index) override;
						void Move(int x, int y) override;
						void SendUpdate(int dx, int dy);

					private:
						uint32_t last_command;

						uint32_t status, resolution, sample_rate;

						int last_x, last_y;
						int button_state;
					};
				}
			}
		}
	}
}

#endif	/* PS2DEVICE_H */

