/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ArmControlCoprocessor.h
 *
 *  Created on: 18 Jun 2014
 *      Author: harry
 */

#ifndef ARMCONTROLCOPROCESSOR_H_
#define ARMCONTROLCOPROCESSOR_H_

#include "abi/devices/arm/core/ArmCoprocessor.h"
#include "abi/devices/arm/core/PMU.h"
#include "util/LogContext.h"

UseLogContext(LogArmCoprocessor);

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class MMU;

			class ArmControlCoprocessor : public ArmCoprocessor
			{
			public:
				ArmControlCoprocessor();

				uint32_t CACHE_SIZE_SELECTION = 0;

				bool is_mmu_enabled() const
				{
					return cp1_M;
				}
				uint32_t get_ttbr() const
				{
					return ttbr;
				}
				uint32_t get_dacr() const
				{
					return dacr;
				}

				void set_dfar(uint32_t new_far)
				{
					LC_DEBUG1(LogArmCoprocessor) << "Writing to FAR " << std::hex << new_far;
					dfar = new_far;
				}

				void set_dfsr(uint32_t new_fsr)
				{
					LC_DEBUG1(LogArmCoprocessor) << "Writing to FSR " << std::hex << new_fsr;
					dfsr = new_fsr;
				}

				void set_ifar(uint32_t new_far)
				{
					LC_DEBUG1(LogArmCoprocessor) << "Writing to FAR " << std::hex << new_far;
					ifar = new_far;
				}

				void set_ifsr(uint32_t new_fsr)
				{
					LC_DEBUG1(LogArmCoprocessor) << "Writing to FSR " << std::hex << new_fsr;
					ifsr = new_fsr;
				}

				/* ARMv5 MMU */
				void set_far(uint32_t new_far)
				{
					LC_DEBUG1(LogArmCoprocessor) << "Writing to FAR " << std::hex << new_far;
					far = new_far;
				}

				void set_fsr(uint32_t new_fsr)
				{
					LC_DEBUG1(LogArmCoprocessor) << "Writing to FSR " << std::hex << new_fsr;
					fsr = new_fsr;
				}

				bool get_cp1_S()
				{
					return cp1_S;
				}
				bool get_cp1_R()
				{
					return cp1_R;
				}

				bool access_cp0(bool is_read, uint32_t &data) override;
				bool access_cp1(bool is_read, uint32_t &data) override;
				bool access_cp2(bool is_read, uint32_t &data) override;
				bool access_cp3(bool is_read, uint32_t &data) override;
				bool access_cp5(bool is_read, uint32_t &data) override;
				bool access_cp6(bool is_read, uint32_t &data) override;
				bool access_cp7(bool is_read, uint32_t &data) override;
				bool access_cp8(bool is_read, uint32_t &data) override;
				bool access_cp9(bool is_read, uint32_t &data) override;
				bool access_cp10(bool is_read, uint32_t &data) override;

				bool access_cp15(bool is_read, uint32_t &data) override;

			private:
				arm::core::PMU pmu;

				bool cp1_M, cp1_S, cp1_R;
				uint32_t ttbr, dacr;

				uint32_t fsr, far;
				uint32_t ifsr, ifar;
				uint32_t dfsr, dfar;

				// TODO: fix this
//				const gensim::RegisterDescriptor *V_descriptor;

				MMU *get_mmu();
			};

		}
	}
}


#endif /* ARMCONTROLCOPROCESSOR_H_ */
