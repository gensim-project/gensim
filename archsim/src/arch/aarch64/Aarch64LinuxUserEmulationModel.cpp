/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/LinuxUserEmulationModel.h"
#include "abi/devices/aarch64/core/AArch64Coprocessor.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "gensim/gensim_decode.h"

UseLogContext(LogEmulationModel)
DeclareChildLogContext(LogEmulationModelAarch64LinuxUser, LogEmulationModel, "Aarch64")

namespace archsim
{
	namespace arch
	{
		namespace aarch64
		{

			class Aarch64DecodeContext : public gensim::DecodeContext
			{
			public:
				Aarch64DecodeContext(const ArchDescriptor &arch) : arch_(arch) {}

				uint32_t DecodeSync(archsim::MemoryInterface& mem_interface, archsim::Address address, uint32_t mode, gensim::BaseDecode *&target) override
				{
					target = arch_.GetISA(mode).GetNewDecode();
					auto result = arch_.GetISA(0).DecodeInstr(address, &mem_interface, *target);

					if((target->ir & 0xff000010) == 0x54000000) {
						target->SetIsPredicated();
					}

					return result;
				}
			private:
				const ArchDescriptor &arch_;
			};

			class Aarch64LinuxUserEmulationModel : public archsim::abi::LinuxUserEmulationModel
			{
			public:
				Aarch64LinuxUserEmulationModel() : archsim::abi::LinuxUserEmulationModel("aarch64", true) {}
				virtual ~Aarch64LinuxUserEmulationModel() {}

				gensim::DecodeContext* GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu) override
				{
					return new archsim::arch::aarch64::Aarch64DecodeContext(cpu.GetArch());
				}
				void HaltCores() override
				{
					UNIMPLEMENTED;
				}
				archsim::abi::ExceptionAction HandleException(archsim::core::thread::ThreadInstance* thread, unsigned int category, unsigned int data) override
				{
					if(category == 3) {
						// emulate system call
						auto bank = thread->GetRegisterFileInterface().GetEntry<uint64_t>("RBX");
						uint64_t* registers = (uint64_t*)bank;

						archsim::abi::SyscallRequest request {0, thread};

						abi::SyscallResponse response;
						response.action = abi::ResumeNext;

						request.syscall = registers[8];
						request.arg0 = registers[0];
						request.arg1 = registers[1];
						request.arg2 = registers[2];
						request.arg3 = registers[3];
						request.arg4 = registers[4];
						request.arg5 = registers[5];

						if (EmulateSyscall(request, response)) {
							registers[0] = response.result;
						} else {
							LC_WARNING(LogEmulationModelAarch64LinuxUser) << "Syscall not supported: " << std::hex << request.syscall;
							registers[0] = -1;
						}

						return response.action;
					}

					// Unhandled exception
					return abi::AbortSimulation;
				}

				bool PrepareBoot(System& system) override
				{
					if(!LinuxUserEmulationModel::PrepareBoot(system)) {
						return false;
					}

					GetMainThread()->GetFeatures().SetFeatureLevel("EMULATE_LINUX_ARCHSIM", 1);

					auto tpid_coprocessor = new archsim::abi::devices::aarch64::core::AArch64Coprocessor();
					GetMainThread()->GetPeripherals().RegisterDevice("TPID", tpid_coprocessor);
					GetMainThread()->GetPeripherals().AttachDevice("TPID", 16);

					return true;
				}

				void PrintStatistics(std::ostream& stream) override
				{
					UNIMPLEMENTED;
				}
			};

		}
	}
}


using archsim::arch::aarch64::Aarch64LinuxUserEmulationModel;
RegisterComponent(archsim::abi::EmulationModel, Aarch64LinuxUserEmulationModel, "aarch64-user", "ARM Linux user emulation model");