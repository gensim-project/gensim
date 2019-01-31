/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SifiveUART.h
 * Author: harry
 *
 * Created on 30 January 2019, 15:10
 */

#ifndef SIFIVECLINT_H
#define SIFIVECLINT_H

#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"
#include "abi/devices/SerialPort.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace riscv
			{
				class SifiveCLINT : public MemoryComponent
				{
				public:
					SifiveCLINT(EmulationModel &parent, Address base_address);
					virtual ~SifiveCLINT();

					bool Initialise() override;
					bool Read(uint32_t offset, uint8_t size, uint32_t& data) override;
					bool Write(uint32_t offset, uint8_t size, uint32_t data) override;

				private:
					archsim::core::thread::ThreadInstance *GetHart(int i);
					void SetMIP(archsim::core::thread::ThreadInstance *thread, bool P);

					COMPONENT_PARAMETER_THREAD(Hart0);

					uint64_t mtimecmp0;

				};
			}
		}
	}
}

#endif /* SIFIVECLINT_H */

