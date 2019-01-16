/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * arch/arm/ArmLinuxUserEmulationModel.cpp
 */
#include "arch/risc-v/RiscVComplianceEmulationModel.h"
#include "arch/risc-v/RiscVDecodeContext.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"
#include "system.h"

using namespace archsim::abi;
using namespace archsim::abi::memory;
using namespace archsim::arch::riscv;

UseLogContext(LogEmulationModelUser);
DeclareChildLogContext(LogEmulationModelRiscVCompliance, LogEmulationModelUser, "RISCV-Compliance");

RegisterComponent(archsim::abi::EmulationModel, RiscVComplianceEmulationModel, "riscv-compliance", "RISC-V Compliance emulation model");

RiscVComplianceEmulationModel::RiscVComplianceEmulationModel() : LinuxUserEmulationModel("riscv", false, AuxVectorEntries("risc-v", 0, 0)) { }

RiscVComplianceEmulationModel::~RiscVComplianceEmulationModel() { }

bool RiscVComplianceEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!LinuxUserEmulationModel::Initialise(system, uarch))
		return false;

	return true;
}

void RiscVComplianceEmulationModel::Destroy()
{
	LinuxUserEmulationModel::Destroy();
}

bool RiscVComplianceEmulationModel::PrepareBoot(System& system)
{
	if (!LinuxUserEmulationModel::PrepareBoot(system))
		return false;

	LC_DEBUG1(LogEmulationModelRiscVCompliance) << "Initialising RISC-V Kernel Helpers";

	memory::guest_addr_t kernel_helper_region = 0xffff0000_ga;
	GetMemoryModel().GetMappingManager()->MapRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[eabi]");

	/* random data */
	GetMemoryModel().Write32(0xffff0000_ga, 0xbabecafe);
	GetMemoryModel().Write32(0xffff0004_ga, 0xdeadbabe);
	GetMemoryModel().Write32(0xffff0008_ga, 0xfeedc0de);
	GetMemoryModel().Write32(0xffff000c_ga, 0xcafedead);

	return true;
}

gensim::DecodeContext* RiscVComplianceEmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu)
{
	return new arch::riscv::RiscVDecodeContext(cpu.GetArch());
}

bool RiscVComplianceEmulationModel::InvokeSignal(int signum, uint32_t next_pc, SignalData* data)
{
	assert(false);

	//TODO

	return false;
}

archsim::abi::ExceptionAction RiscVComplianceEmulationModel::HandleException(archsim::core::thread::ThreadInstance *cpu, unsigned int category, unsigned int data)
{
	if(category == 1024) {
		GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, (void*)(uint64_t)0);
		return archsim::abi::ResumeNext;
	}

	if(category == 0) {
		// Reduced set of system calls:
		// 0: write(buffer, len)
		// 1: write signature file
		// 2: exit OK
		// 3: exit error

		uint32_t* registers = (uint32_t*)cpu->GetRegisterFile();

		uint32_t syscall_no = registers[17];
		uint32_t syscall_arg0 = registers[10];
		uint32_t syscall_arg1 = registers[11];

		switch(syscall_no) {
			case 0: {
				LC_DEBUG1(LogEmulationModelRiscVCompliance) << "Writing " << syscall_arg1 << " bytes from " << Address(syscall_arg0);
				std::vector<char> buffer;
				buffer.resize(syscall_arg1);

				GetMemoryModel().Read(Address(syscall_arg0), (uint8_t*)buffer.data(), syscall_arg1);
				fwrite(buffer.data(), syscall_arg1, 1, stdout);

				return archsim::abi::ResumeNext;
			}
			case 1: {
				Address sig_start, sig_end;

				bool success = ResolveSymbol("begin_sig_data",sig_start);
				success &= ResolveSymbol("end_sig_data",sig_end);

				if(success) {
					LC_INFO(LogEmulationModelRiscVCompliance) << "Writing out signature data:";

					for(Address i = sig_start; i < sig_end; i += 4) {
						uint32_t data;
						GetMemoryModel().Read32(i, data);

						LC_INFO(LogEmulationModelRiscVCompliance) << "SIG: " << std::hex << std::setw(8) << std::setfill('0') << data;
					}

				} else {
					LC_ERROR(LogEmulationModelRiscVCompliance) << "Could not find signature symbols";
				}

				return archsim::abi::ResumeNext;
			}
			case 2:
				GetSystem().exit_code = 0;
				cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
				return AbortSimulation;
			case 3:
				GetSystem().exit_code = 1;
				cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
				return AbortSimulation;
			default:
				LC_ERROR(LogEmulationModelRiscVCompliance) << "Unrecognized system call " << syscall_no;
				return AbortSimulation;
		}

	}

	return AbortSimulation;
}
