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
					COMPONENT_PARAMETER_STRING(HartConfig);

					std::vector<uint32_t> interrupt_priorities_;

					class hart_config
					{
					public:
						hart_config() : priority_threshold_m(0), priority_threshold_s(0)
						{
							enable_m.resize(2);
							enable_s.resize(2);
						}

						uint32_t priority_threshold_m;
						uint32_t priority_threshold_s;

						uint32_t threshold_m, threshold_s;

						std::vector<uint32_t> enable_m, enable_s;
					};

					std::vector<hart_config> hart_config_;

					bool ReadPriority(uint32_t offset, uint64_t &data);
					bool ReadEnable(uint32_t offset, uint64_t &data);
					bool ReadPending(uint32_t offset, uint64_t &data);
					bool ReadThreshold(uint32_t offset, uint64_t &data);

					bool WritePriority(uint32_t offset, uint64_t data);
					bool WriteEnable(uint32_t offset, uint64_t data);
					bool WritePending(uint32_t offset, uint64_t data);
					bool WriteThreshold(uint32_t offset, uint64_t data);
				};
			}
		}
	}
}

#endif /* SIFIVEPLIC_H */

