/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "abi/devices/MMU.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{

			class RiscVMMU : public archsim::abi::devices::MMU
			{
			public:
				bool Initialise() override;

				MMU::TranslateResult Translate(archsim::core::thread::ThreadInstance* cpu, Address virt_addr, Address& phys_addr, archsim::abi::devices::AccessInfo info) override;
				const archsim::abi::devices::PageInfo GetInfo(Address virt_addr) override;

			};
		}
	}
}