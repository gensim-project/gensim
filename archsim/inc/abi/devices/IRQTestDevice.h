/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * IRQTestDevice.h
 *
 *  Created on: 20 Jan 2014
 *      Author: harry
 */

#ifndef IRQTESTDEVICE_H_
#define IRQTESTDEVICE_H_

#include "abi/devices/Device.h"
#include "abi/devices/IRQController.h"
#include "gensim/gensim_processor.h"

#include <chrono>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class IRQTestDevice : public ArmCoprocessor
			{
			public:
				IRQTestDevice();

				virtual bool Initialise();
				bool Configure(gensim::Processor *core, std::map<std::string, std::string>& settings);
				virtual bool access_cp0(bool is_read, uint32_t &data);

				void SetIRQLine(IRQLine *irq);

				void Timeout();

				typedef std::chrono::high_resolution_clock timer_t;
				typedef timer_t::duration time_difference_t;
				typedef timer_t::time_point time_point_t;

				int GetComponentID();

			private:
				void write_out_latencies();

				gensim::Processor *core;
				IRQLine *irq;
				time_point_t oneshot_issued;
				time_point_t irq_sent;
				uint32_t interrupt_delay;

				struct latency_packet {
					time_difference_t latency;
					time_difference_t delay;
					uint32_t pc;
					bool jit;
				};

				std::vector<latency_packet> latencies;
			};

		}
	}
}



#endif /* IRQTESTDEVICE_H_ */
