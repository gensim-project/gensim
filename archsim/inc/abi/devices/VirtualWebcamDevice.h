/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * VirtualWebcamDevice.h
 *
 *  Created on: 7 Mar 2014
 *      Author: harry
 */

#ifndef VIRTUALWEBCAMDEVICE_H_
#define VIRTUALWEBCAMDEVICE_H_

#include "concurrent/Thread.h"
#include "abi/devices/Component.h"

struct VirtualWebcamContext;

namespace archsim
{
	namespace abi
	{
		class EmulationModel;

		namespace devices
		{

			class VirtualWebcamDevice : public SystemComponent, public archsim::concurrent::Thread
			{
			public:
				VirtualWebcamDevice(EmulationModel& model);
				~VirtualWebcamDevice();

				int GetComponentID();

				bool Attach();
			private:
				void run();

				EmulationModel &emulation_model;

				struct VirtualWebcamContext *webcam;
				bool terminated;
			};

		}
	}
}

#endif /* VIRTUALWEBCAMDEVICE_H_ */
