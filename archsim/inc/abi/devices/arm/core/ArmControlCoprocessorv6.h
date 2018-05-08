/*
 * ArmControlCoprocessorv6.h
 *
 *  Created on: 18 Jan 2016
 *      Author: kuba
 */

#ifndef ARCSIM_INC_ABI_DEVICES_ARM_CORE_ARMCONTROLCOPROCESSORV6_H_
#define ARCSIM_INC_ABI_DEVICES_ARM_CORE_ARMCONTROLCOPROCESSORV6_H_

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

			class ArmControlCoprocessorv6 : public ArmCoprocessor
			{
			public:
				ArmControlCoprocessorv6();

				uint32_t CACHE_SIZE_SELECTION = 0;
				uint32_t PRIMARY_REGION_REMAP = 0x00098AA4;
				uint32_t NORMAL_REGION_REMAP = 0x44E048E0;

				bool is_mmu_enabled() const
				{
					return cp1_M;
				}
				uint32_t get_ttbr() const
				{
					return ttbr0;
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
				bool access_cp13(bool is_read, uint32_t &data) override;
				bool access_cp15(bool is_read, uint32_t &data) override;

			private:
				arm::core::PMU pmu;

				bool cp1_M, cp1_S, cp1_R;
				uint32_t ttbr0, ttbr1, ttbcr, dacr;
				uint32_t tpidrurw, tpidruro, tpidrprw, contextidr;

				uint32_t fsr, far;
				uint32_t ifsr, ifar;
				uint32_t dfsr, dfar;

				uint32_t cpacr, actlr;

				uint32_t sctl_word;

				bool TRE,AFE;

				uint8_t *V_ptr;

				MMU *get_mmu();
			};

		}
	}
}


#endif /* ARCSIM_INC_ABI_DEVICES_ARM_CORE_ARMCONTROLCOPROCESSORV6_H_ */
