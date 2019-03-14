/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * PL011.h
 *
 *  Created on: 10 Jan 2014
 *      Author: harry
 */

#ifndef PL011_H_
#define PL011_H_

#include "abi/devices/Component.h"
#include "abi/devices/SerialPort.h"
#include "abi/devices/IRQController.h"


#include <deque>

#include <stdint.h>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class IRQLine;
		}
		namespace external
		{
			namespace arm
			{
				using namespace archsim::abi::devices;

				class PL011 : public MemoryComponent
				{
				public:
					PL011(EmulationModel &parent, Address base_address);
					~PL011();

					bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
					bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

					void EnqueueChar(char c);

					bool Initialise() override;

				private:
					uint32_t control_word;
					uint32_t baud_rate, fractional_baud, line_control;
					uint32_t irq_mask, irq_status;
					uint32_t ifl;

					COMPONENT_PARAMETER_ENTRY(IRQLine, IRQLine, IRQLine);
					COMPONENT_PARAMETER_ENTRY(SerialPort, SerialPort, SerialPort);

					archsim::abi::devices::AsyncSerial *GetSerial();

					std::deque<char> fifo;

					void RaiseTxIRQ();
					void RaiseRxIRQ();

					void CheckIRQs();

					archsim::abi::devices::AsyncSerial *serial;
				};
			}
		}
	}
}

#endif /* PL011_H_ */
