/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "abi/LinuxSystemEmulationModel.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{

			class RiscVSifiveFU540EmulationModel : public archsim::abi::LinuxSystemEmulationModel
			{
			public:
				RiscVSifiveFU540EmulationModel(int xlen);

				gensim::DecodeContext* GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu) override;

				bool Initialise(System& system, archsim::uarch::uArch& uarch) override;

				archsim::abi::ExceptionAction HandleException(archsim::core::thread::ThreadInstance* cpu, uint64_t category, uint64_t data) override;
				archsim::abi::ExceptionAction HandleMemoryFault(archsim::core::thread::ThreadInstance& thread, archsim::MemoryInterface& interface, archsim::Address address) override;
				void HandleInterrupt(archsim::core::thread::ThreadInstance* thread, archsim::abi::devices::CPUIRQLine* irq) override;


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
