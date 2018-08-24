/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * IRQTestDevice.cpp
 *
 *  Created on: 20 Jan 2014
 *      Author: harry
 */

#include "abi/devices/IRQTestDevice.h"
#include "abi/devices/PeripheralManager.h"
#include "abi/EmulationModel.h"
#include "gensim/gensim_processor.h"



namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			RegisterComponent(archsim::abi::devices::Device, IRQTestDevice, "irqtestdevice", "IRQ test device for LCTES paper");

			void irq_test_device_callback(util::timing::TimerState *timer,  void *data)
			{
				IRQTestDevice *dev = (IRQTestDevice*)data;

				dev->Timeout();
			}

			IRQTestDevice::IRQTestDevice() : ArmCoprocessor()
			{

			}

			bool IRQTestDevice::Initialise()
			{
				return true;
			}

			bool IRQTestDevice::Configure(gensim::Processor *core, std::map<std::string, std::string>& settings)
			{
				this->core = core;
				SetIRQLine(core->GetIRQLine(atoi(settings.at("irq").c_str())));

				return true;
			}

			void IRQTestDevice::SetIRQLine(IRQLine *irq)
			{
				this->irq = irq;
			}

			bool IRQTestDevice::access_cp0(bool is_read, uint32_t &data)
			{
				switch(opc1) {
					case 0x0:
						oneshot_issued = timer_t::now();
						Manager->emu_model.timer_mgr.RegisterOneshot(interrupt_delay, irq_test_device_callback, this);
						return true;
					case 0x1: {
						time_point_t refp = timer_t::now();
						time_difference_t oneshot_delay = irq_sent - oneshot_issued;
						time_difference_t latency = refp - irq_sent;

						latency_packet pkt;
						pkt.latency = latency;
						pkt.delay = oneshot_delay;
						pkt.jit = core->cur_exec_mode == gensim::Processor::kExecModeNative;

						uint32_t *regs = (uint32_t *)core->GetRegisterBankDescriptor("RB").BankDataStart;
						pkt.pc = regs[14];

						latencies.push_back(pkt);
						return true;
					}
					case 0x2:
						write_out_latencies();
						return true;
					case 0x3:
						interrupt_delay = data;
						return true;
				}
				return false;
			}

			void IRQTestDevice::write_out_latencies()
			{
				FILE *f = fopen("latencies", "w");
				for(const auto pkt : latencies) {
					std::chrono::microseconds us_latency = std::chrono::duration_cast<std::chrono::microseconds>(pkt.latency);
					std::chrono::microseconds us_delay = std::chrono::duration_cast<std::chrono::microseconds>(pkt.delay);
					uint64_t latency_count = us_latency.count();
					uint64_t delay_count = us_delay.count();
					fprintf(f, "%c %08x: %lu %lu\n", pkt.jit ? '*' : ' ', pkt.pc, delay_count, latency_count);
				}
				fclose(f);
			}

			void IRQTestDevice::Timeout()
			{
				irq_sent = timer_t::now();
				irq->Assert();
			}

			int IRQTestDevice::GetComponentID()
			{
				return 0x28493208;
			}

		}
	}
}
