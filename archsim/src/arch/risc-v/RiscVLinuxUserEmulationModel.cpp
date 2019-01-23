/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * arch/arm/ArmLinuxUserEmulationModel.cpp
 */
#include "arch/risc-v/RiscVLinuxUserEmulationModel.h"
#include "arch/risc-v/RiscVDecodeContext.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"
#include "system.h"

#ifdef CONFIG_GFX
#ifdef CONFIG_SDL
#include "abi/devices/gfx/SDLScreen.h"
#endif
#include "abi/devices/generic/DoomDevice.h"
#endif

using namespace archsim::abi;
using namespace archsim::abi::memory;
using namespace archsim::arch::riscv;

UseLogContext(LogEmulationModelUser);
DeclareChildLogContext(LogEmulationModelRiscVLinux, LogEmulationModelUser, "RISCV-Linux");

class RVLinuxUserEmulationModel32 : public RiscVLinuxUserEmulationModel
{
public:
	RVLinuxUserEmulationModel32() : RiscVLinuxUserEmulationModel(false) {}
};
class RVLinuxUserEmulationModel64 : public RiscVLinuxUserEmulationModel
{
public:
	RVLinuxUserEmulationModel64() : RiscVLinuxUserEmulationModel(true) {}
};


RegisterComponent(archsim::abi::EmulationModel, RVLinuxUserEmulationModel32, "riscv32-user", "ARM Linux user emulation model");
RegisterComponent(archsim::abi::EmulationModel, RVLinuxUserEmulationModel64, "riscv64-user", "ARM Linux user emulation model");

RiscVLinuxUserEmulationModel::RiscVLinuxUserEmulationModel(bool rv64) : LinuxUserEmulationModel("riscv", rv64, AuxVectorEntries("risc-v", 0, 0)) { }

RiscVLinuxUserEmulationModel::~RiscVLinuxUserEmulationModel() { }

bool RiscVLinuxUserEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!LinuxUserEmulationModel::Initialise(system, uarch))
		return false;

	return true;
}

void RiscVLinuxUserEmulationModel::Destroy()
{
	LinuxUserEmulationModel::Destroy();
}

bool RiscVLinuxUserEmulationModel::PrepareBoot(System& system)
{
	if (!LinuxUserEmulationModel::PrepareBoot(system))
		return false;

	LC_DEBUG1(LogEmulationModelRiscVLinux) << "Initialising RISC-V Kernel Helpers";

	memory::guest_addr_t kernel_helper_region = 0xffff0000_ga;
	GetMemoryModel().GetMappingManager()->MapRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[eabi]");

	/* random data */
	GetMemoryModel().Write32(0xffff0000_ga, 0xbabecafe);
	GetMemoryModel().Write32(0xffff0004_ga, 0xdeadbabe);
	GetMemoryModel().Write32(0xffff0008_ga, 0xfeedc0de);
	GetMemoryModel().Write32(0xffff000c_ga, 0xcafedead);

	return true;
}

gensim::DecodeContext* RiscVLinuxUserEmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu)
{
	return new arch::riscv::RiscVDecodeContext(cpu.GetArch());
}

bool RiscVLinuxUserEmulationModel::InvokeSignal(int signum, uint32_t next_pc, SignalData* data)
{
	assert(false);

	//TODO

	return false;
}

archsim::abi::ExceptionAction RiscVLinuxUserEmulationModel::HandleException(archsim::core::thread::ThreadInstance *cpu, unsigned int category, unsigned int data)
{
	if(category == 1024) {
		GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, (void*)(uint64_t)0);
		return archsim::abi::ResumeNext;
	}

	if(category == 0) {
		archsim::abi::SyscallResponse response;
		response.action = ResumeNext;

		archsim::abi::SyscallRequest request {0, cpu, 0, 0, 0, 0, 0, 0};

		if(Is64BitBinary()) {
			uint64_t* registers = (uint64_t*)cpu->GetRegisterFile();

			request.syscall = registers[17];

			request.arg0 = registers[10];
			request.arg1 = registers[11];
			request.arg2 = registers[12];
			request.arg3 = registers[13];
			request.arg4 = registers[14];
			request.arg5 = registers[15];
		} else {
			uint32_t* registers = (uint32_t*)cpu->GetRegisterFile();

			request.syscall = registers[17];

			request.arg0 = registers[10];
			request.arg1 = registers[11];
			request.arg2 = registers[12];
			request.arg3 = registers[13];
			request.arg4 = registers[14];
			request.arg5 = registers[15];
		}

		if(EmulateSyscall(request, response)) {

		} else {
			LC_ERROR(LogEmulationModelRiscVLinux) << "Syscall not supported: " << std::hex << "0x" << request.syscall << "(" << std::dec << request.syscall << ")";
			response.result = -1;
		}

		if(Is64BitBinary()) {
			uint64_t* registers = (uint64_t*)cpu->GetRegisterFile();
			registers[10] = response.result;
		} else {
			uint32_t* registers = (uint32_t*)cpu->GetRegisterFile();
			registers[10] = response.result;
		}

		// xxx arm hax
		// currently a syscall cannot return an action other than resume, so
		// we need to exit manually here.
		if(request.syscall == 93) {
			cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
			return AbortSimulation;
		}

		return response.action;
	}

	return AbortSimulation;
}
