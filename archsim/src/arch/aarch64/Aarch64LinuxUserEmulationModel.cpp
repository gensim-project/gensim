/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/LinuxUserEmulationModel.h"
#include "util/ComponentManager.h"

namespace archsim {
	namespace arch {
		namespace aarch64 {

			class Aarch64DecodeContext : public gensim::DecodeContext 
			{
			public:
				Aarch64DecodeContext(const ArchDescriptor &arch) : arch_(arch) {}
				
				uint32_t DecodeSync(archsim::MemoryInterface& mem_interface, archsim::Address address, uint32_t mode, gensim::BaseDecode& target) override
				{
					return arch_.GetISA(0).DecodeInstr(address, &mem_interface, target);
				}
			private:
				const ArchDescriptor &arch_;
			};
			
			class Aarch64LinuxUserEmulationModel : public archsim::abi::LinuxUserEmulationModel
			{
			public:
				Aarch64LinuxUserEmulationModel() : archsim::abi::LinuxUserEmulationModel("aarch64", true) {}
				virtual ~Aarch64LinuxUserEmulationModel() {}
				
				gensim::DecodeContext* GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu) override { return new archsim::arch::aarch64::Aarch64DecodeContext(cpu.GetArch()); }
				void HaltCores() override { UNIMPLEMENTED; }
				archsim::abi::ExceptionAction HandleException(archsim::core::thread::ThreadInstance* cpu, unsigned int category, unsigned int data) override { UNIMPLEMENTED; }
				bool PrepareBoot(System& system) override { return LinuxUserEmulationModel::PrepareBoot(system); }
				void PrintStatistics(std::ostream& stream) override { UNIMPLEMENTED; }
			};
	
		}
	}
}


using archsim::arch::aarch64::Aarch64LinuxUserEmulationModel;
RegisterComponent(archsim::abi::EmulationModel, Aarch64LinuxUserEmulationModel, "aarch64-user", "ARM Linux user emulation model");