/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/arm/special/SimulatorCacheControlCoprocessor.h"

#include "abi/devices/PeripheralManager.h"
#include "abi/devices/DeviceManager.h"

#include "abi/EmulationModel.h"
#include "abi/SystemEmulationModel.h"

//#include "uarch/uArch.h"
//#include "uarch/cache/CacheGeometry.h"
#include "abi/memory/system/CacheBasedSystemMemoryModel.h"
#include "system.h"

#include <iostream>
#include <ctime>
#include <chrono>

#include <libtrace/TraceSource.h>

using namespace archsim::abi::devices;

SimulatorCacheControlCoprocessor::SimulatorCacheControlCoprocessor() : scptext_char(0)
{

}

bool SimulatorCacheControlCoprocessor::Initialise()
{
	return true;
}

std::chrono::high_resolution_clock::time_point start_time;
uint32_t last_duration;
uint64_t start_flushes;

bool SimulatorCacheControlCoprocessor::access_cp0(bool is_read, uint32_t& data)
{
	if (is_read) {
		data = 0;
//		switch (rm) {
//			case 6: // Last Duration after Timer Stop
//				data = last_duration;
//				break;
//			case 7: // Dynamic Interrupt Checks
//				if (opc2 == 0)
//					data = (uint32_t)(Manager->cpu.metrics.interrupt_checks.get_value() & 0xffffffff);
//				else if (opc2 == 1)
//					data = (uint32_t)((Manager->cpu.metrics.interrupt_checks.get_value() >> 32) & 0xffffffff);
//				break;
//			case 8: // Interrupts Serviced
//				if (opc2 == 0)
//					data = (uint32_t)(Manager->cpu.metrics.interrupts_serviced.get_value() & 0xffffffff);
//				else if (opc2 == 1)
//					data = (uint32_t)((Manager->cpu.metrics.interrupts_serviced.get_value() >> 32) & 0xffffffff);
//				break;
//			case 9: // Instruction Count
//				if (opc2 == 0)
//					data = (uint32_t)(Manager->cpu.instructions() & 0xffffffff);
//				else if (opc2 == 1)
//					data = (uint32_t)((Manager->cpu.instructions() >> 32) & 0xffffffff);
//				break;
//			case 12: // I/O
//				if(opc2 == 0)
//					data = archsim::options::SCPText.GetValue().length();
//				else {
//					if(scptext_char < archsim::options::SCPText.GetValue().length())
//						data = archsim::options::SCPText.GetValue().at(scptext_char);
//					else data = 0;
//				}
//		}
	} else {
		switch (rm) {
			case 0:
//				Manager->cpu.GetEmulationModel().GetuArch().GetCacheGeometry().ResetStatistics();
				break;
			case 1:
				std::cerr << "*************************" << std::endl;
//				Manager->cpu.GetEmulationModel().GetuArch().GetCacheGeometry().PrintStatistics(std::cerr);
				std::cerr << "*************************" << std::endl;
				break;
			case 2: {
				std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

				std::cerr << "*************************" << std::endl;
				std::cerr << "Host Time: " << ctime(&now) << std::endl;
				std::cerr << "*************************" << std::endl;
				break;
			}
			case 3:
//				Manager->cpu.InitialiseTracing();
				break;
			case 4:
				std::cerr << "*************************" << std::endl;
//				std::cerr << "Interrupt Checks:    " << Manager->cpu.metrics.interrupt_checks.get_value() << std::endl;
//				std::cerr << "Interrupts Serviced: " << Manager->cpu.metrics.interrupts_serviced.get_value() << std::endl;
				std::cerr << "*************************" << std::endl;
				break;

			case 5: // Start Timer
				start_time = std::chrono::high_resolution_clock::now();
//			start_flushes = ((archsim::abi::memory::SystemMemoryModel&)Manager->cpu.GetMemoryModel()).num_flushes;
				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::UserEvent1, NULL);
				break;

			case 6: { // Stop Timer
				std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
				last_duration = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

//			uint64_t end_flushes = ((archsim::abi::memory::SystemMemoryModel&)Manager->cpu.GetMemoryModel()).num_flushes;

				std::cerr << "*************************" << std::endl;
				std::cerr << "Duration: " << last_duration << "ms" << std::endl;
//			std::cerr << "Flushes: " << end_flushes-start_flushes  << std::endl;
				std::cerr << "*************************" << std::endl;

				Manager->GetEmulationModel()->GetSystem().GetPubSub().Publish(PubSubType::UserEvent2, NULL);
				break;
			}

			case 7: { // Halt system
				Manager->GetEmulationModel()->GetSystem().HaltSimulation();
				break;
			}

			case 10:
				if (opc2 == 0) {
//					if (!Manager->cpu.HasTraceManager()) {
//						Manager->cpu.InitialiseTracing();
//					}
//
//					Manager->cpu.StartTracing();
//
//					Manager->cpu.GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::FlushAllTranslations, nullptr);
//				} else if (opc2 == 1) {
//					Manager->cpu.StopTracing();
//
//					Manager->cpu.GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::FlushAllTranslations, nullptr);
				}

				break;

			case 11: {

				break;
			}
			case 12: // I/O
				if(opc2 == 0) scptext_char = data;
				else fprintf(stderr, "%c", data & 0xff);
				break;

			case 13: // Tick scale
				archsim::options::TickScale.SetValue(data);
				break;
		}
	}

	return true;
}

bool SimulatorCacheControlCoprocessor::access_cp1(bool is_read, uint32_t &data)
{
	if(is_read) {

	} else {
		switch(rm) {
			//Enable/Disable tracing
			case 0:
				if (opc2 == 0) {
//					if (!Manager->cpu.HasTraceManager()) {
//						Manager->cpu.InitialiseTracing();
//					}
//
//					Manager->cpu.StartTracing();
//
//					if (Manager->cpu.GetEmulationModel().GetSystem().HaveTranslationManager()) {
//						Manager->cpu.GetEmulationModel().GetSystem().GetTranslationManager().Invalidate();
//					}
				} else if (opc2 == 1) {
//					Manager->cpu.StopTracing();
//
//					if (Manager->cpu.GetEmulationModel().GetSystem().HaveTranslationManager()) {
//						Manager->cpu.GetEmulationModel().GetSystem().GetTranslationManager().Invalidate();
//					}
				}
				break;

			case 1:
				if(opc2 == 0) {
//					if(Manager->cpu.HasTraceManager()) Manager->cpu.GetTraceManager()->Suppress();
				} else {
//					if(Manager->cpu.HasTraceManager()) Manager->cpu.GetTraceManager()->Unsuppress();
				}

		}
	}
	return true;
}
