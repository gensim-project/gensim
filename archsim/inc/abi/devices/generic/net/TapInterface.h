/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   TapInterface.h
 * Author: harry
 *
 * Created on 15 November 2016, 16:53
 */

#ifndef TAPINTERFACE_H
#define TAPINTERFACE_H

#include "NetworkInterface.h"

#include <thread>
#include <cstdint>
#include <string>

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

					class TapInterface : public NetworkInterface
					{
					public:
						TapInterface(const std::string &tap_device);
						virtual ~TapInterface();

						virtual bool transmit_packet(const uint8_t *buffer, uint32_t length) override;

						virtual bool start() override;
						virtual void stop() override;



					private:
						static void receive_thread_trampoline(TapInterface *obj);
						void receive_thread_proc();

						bool _terminate;
						const std::string _tap_device;
						int _tap_fd;
						std::thread *_receive_thread;
					};

				}
			}
		}
	}
}

#endif /* TAPINTERFACE_H */

