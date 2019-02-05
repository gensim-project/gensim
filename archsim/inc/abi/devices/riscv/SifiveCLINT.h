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
#include "abi/devices/generic/timing/TickConsumer.h"
#include "abi/devices/generic/timing/TickSource.h"
#include "system.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace riscv
			{
				class SifiveCLINT;

				/*
				 * Class which manages mtimecmp and the associated interrupt
				 * for a single hart.
				 */
				class CLINTTimer : public archsim::abi::devices::timing::TickConsumer
				{
				public:
					CLINTTimer(archsim::core::thread::ThreadInstance *hart, SifiveCLINT *clint);

					void Tick(uint32_t tick_periods) override;
					void SetCmp(uint64_t cmp);
					uint64_t GetCmp() const;

				private:
					void CheckTick();

					uint64_t cmp_;
					archsim::core::thread::ThreadInstance *hart_;
					SifiveCLINT *clint_;
				};

				class SifiveCLINT : public MemoryComponent
				{
				public:
					SifiveCLINT(EmulationModel &parent, Address base_address);
					virtual ~SifiveCLINT();

					bool Initialise() override;
					bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
					bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

					uint64_t GetTimer();
					CLINTTimer *GetHartTimer(int i);

					archsim::core::thread::ThreadInstance *GetHart(int i);
					archsim::arch::riscv::RiscVSystemCoprocessor *GetCoprocessor(int i);
				private:

					void SetMIP(archsim::core::thread::ThreadInstance *thread, bool P);

					COMPONENT_PARAMETER_THREAD(Hart0);

					archsim::abi::devices::timing::TickSource *tick_source_;

					std::vector<CLINTTimer*> timers_;

				};
			}
		}
	}
}

#endif /* SIFIVECLINT_H */

