/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   AArch64Coprocessor.h
 * Author: harry
 *
 * Created on 28 June 2018, 10:18
 */

#ifndef AARCH64COPROCESSOR_H
#define AARCH64COPROCESSOR_H

#include "abi/devices/Device.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace aarch64
			{
				namespace core
				{
					class AArch64Coprocessor : public archsim::abi::devices::Device
					{
					public:
						AArch64Coprocessor();

						bool Initialise() override;

						bool Read64(uint32_t address, uint64_t& data) override;
						bool Write64(uint32_t address, uint64_t data) override;

					private:
						uint64_t tpidr_el0_;
					};
				}
			}
		}
	}
}

#endif /* AARCH64COPROCESSOR_H */

