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
					int GetHartCount() const
					{
						return 2;    // todo: support multiple harts
					}

					void UpdateIRQ();

					COMPONENT_PARAMETER_THREAD(Hart0);
					COMPONENT_PARAMETER_THREAD(Hart1);
					COMPONENT_PARAMETER_STRING(HartConfig);

					std::vector<uint32_t> interrupt_priorities_;
					std::vector<uint32_t> interrupt_pending_;
					std::vector<uint32_t> interrupt_claimed_;

					class hart_context
					{
					public:
						hart_context(uint32_t hart, char context) : hart(hart), context(context), threshold(0), busy(0)
						{
							enable.resize(2);
						}

						uint32_t hart;
						char context;

						uint32_t threshold;
						std::vector<uint32_t> enable;

						bool busy;
					};

					std::vector<hart_context> hart_config_;
					std::mutex lock_;

					uint32_t ClaimInterrupt(hart_context &context);
					void CompleteInterrupt(uint32_t irq);

					void ClearPendingInterrupt(uint32_t index);
					void RaiseEIPOnHart(uint32_t hart, char context);
					void SetupContexts(const std::string &config);

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

