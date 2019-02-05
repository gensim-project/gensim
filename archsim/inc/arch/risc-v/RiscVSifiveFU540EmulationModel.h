/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "arch/risc-v/RiscVSystemEmulationModel.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{

			class RiscVSifiveFU540EmulationModel : public archsim::arch::riscv::RiscVSystemEmulationModel
			{
			public:
				RiscVSifiveFU540EmulationModel();

				bool Initialise(System& system, archsim::uarch::uArch& uarch) override;

				bool InstallDevices() override;
				void DestroyDevices() override;

				bool InstallPlatform(archsim::abi::loader::BinaryLoader& loader) override;
				bool PrepareCore(archsim::core::thread::ThreadInstance& core) override;

			private:
				bool InstallCoreDevices();
				bool InstallPlatformDevices();
				bool InstallBootloader(archsim::abi::loader::BinaryLoader &loader);
			};

		}
	}
}
