/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SifiveUART.h
 * Author: harry
 *
 * Created on 30 January 2019, 15:10
 */

#ifndef SIFIVEUART_H
#define SIFIVEUART_H

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
				class SifiveUART : public MemoryComponent
				{
				public:
					SifiveUART(EmulationModel &parent, Address base_address);
					virtual ~SifiveUART();

					bool Initialise() override;
					bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
					bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

					bool EnqueueChar(char c);

				private:
					uint32_t txctrl_, rxctrl_;

					COMPONENT_PARAMETER_ENTRY(IRQLine, IRQLine, IRQLine);
					COMPONENT_PARAMETER_ENTRY(SerialPort, SerialPort, SerialPort);

					archsim::abi::devices::AsyncSerial *GetSerial();
					archsim::abi::devices::AsyncSerial *serial_;
				};
			}
		}
	}
}

#endif /* SIFIVEUART_H */

