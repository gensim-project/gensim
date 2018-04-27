/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   NetworkInterface.h
 * Author: harry
 *
 * Created on 15 November 2016, 16:44
 */

#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <cstdint>
#include <functional>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				namespace net
				{

					using NetworkInterfaceReceiveCallback = std::function<void (const uint8_t *buffer, uint32_t length)>;
					
					class NetworkInterface
					{
					public:

						NetworkInterface() : _receive_callback(nullptr)
						{
						}

						virtual ~NetworkInterface()
						{
						}

						void attach(const NetworkInterfaceReceiveCallback& callback)
						{
							_receive_callback = callback;
						}

						virtual bool transmit_packet(const uint8_t *buffer, uint32_t length) = 0;

						virtual bool start() = 0;
						virtual void stop() = 0;

					protected:
						void invoke_receive(const uint8_t *buffer, uint32_t length);

					private:
						NetworkInterfaceReceiveCallback _receive_callback;
					};

				}
			}
		}
	}
}

#endif /* NETWORKINTERFACE_H */

