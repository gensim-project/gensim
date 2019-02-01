/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SifiveUART.h
 * Author: harry
 *
 * Created on 30 January 2019, 15:10
 */

#ifndef SIFIVEPLIC_H
#define SIFIVEPLIC_H

#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace riscv
			{
				class SifivePLIC : public MemoryComponent, public IRQController
				{
				public:
					SifivePLIC(EmulationModel &parent, Address base_address);
					virtual ~SifivePLIC();

					bool Initialise() override;
					bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
					bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

					bool AssertLine(uint32_t line) override;
					bool RescindLine(uint32_t line) override;

				private:
					archsim::core::thread::ThreadInstance *GetHart(int i);

					COMPONENT_PARAMETER_THREAD(Hart0);

					std::vector<uint32_t> interrupt_priorities_;
					uint32_t hart0_m_priority_thresholds_;
				};
			}
		}
	}
}

#endif /* SIFIVEPLIC_H */

