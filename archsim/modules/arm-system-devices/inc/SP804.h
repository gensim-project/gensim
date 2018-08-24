/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SP804.h
 * Author: s0457958
 */

#ifndef SP804_H
#define SP804_H

#include "PrimecellRegisterDevice.h"
#include "abi/devices/generic/timing/TickConsumer.h"
#include "abi/devices/IRQController.h"
#include "util/TimerManager.h"
#include "concurrent/Thread.h"

// ARM Dual Timer Module (SP804)
// Consists of two 32/16 bit count-down timers which generate interrupts on reaching 0

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
				class SP804 : public PrimecellRegisterDevice, public timing::TickConsumer
				{
				public:
					SP804(EmulationModel &parent, Address base_address);
					~SP804();

					bool Initialise() override;

					bool Read(uint32_t offset, uint8_t size, uint32_t& data) override;
					bool Write(uint32_t offset, uint8_t size, uint32_t data) override;

					void Suspend()
					{
						suspended = true;
					}
					void Resume()
					{
						suspended = false;
					};

					class InternalTimer
					{
					public:
						uint8_t id;

						InternalTimer();

						bool ReadRegister(uint32_t offset, uint32_t& data);
						bool WriteRegister(uint32_t offset, uint32_t data);

						inline void SetManager(util::timing::TimerManager& mgr)
						{
							this->mgr = &mgr;
						}
						inline void SetOwner(SP804& owner)
						{
							this->owner = &owner;
						}

						inline uint32_t GetISR() const
						{
							return isr;
						}
						inline bool IsIRQEnabled() const
						{
							return control_reg.bits.int_en;
						}

						void Tick(uint32_t ticks);
						void TickFile();

						inline bool IsEnabled() const
						{
							return enabled;
						}

						void GetNextFileTick();

					private:
						void Update();
						void InitialisePeriod();

						util::timing::TimerManager *mgr;
						SP804 *owner;

						volatile bool enabled;

						uint32_t load_value;
						uint32_t current_value;
						uint32_t internal_prescale;

						uint64_t ticker, next_irq;

						volatile uint32_t isr;

						union {
							uint32_t value;
							struct {
								uint8_t one_shot : 1;
								uint8_t size : 1;
								uint8_t prescale : 2;
								uint8_t rsvd : 1;
								uint8_t int_en : 1;
								uint8_t mode : 1;
								uint8_t enable : 1;
							} __attribute__((packed)) bits;
						} control_reg;
					};

					void Tick(uint32_t tick_periods) override;

				protected:
					void tick();

				private:
					void UpdateIRQ();

					COMPONENT_PARAMETER_ENTRY(IRQLine, IRQLine, IRQLine);
					InternalTimer timers[2];

					uint32_t ticks, calibrated_ticks, recalibrate, total_overshoot, overshoot_samples;
					bool suspended;
					uint32_t prescale;

					EmulationModel &emu_model;
				};
			}
		}
	}
}


#endif
