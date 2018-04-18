/*
 * arch/arm/ArmLinuxUserEmulationModel.cpp
 */
#include "arch/risc-v/RiscVLinuxUserEmulationModel.h"
#include "arch/risc-v/RiscVDecodeContext.h"

#include "gensim/gensim_processor.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"

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

RegisterComponent(archsim::abi::EmulationModel, RiscVLinuxUserEmulationModel, "riscv-user", "ARM Linux user emulation model");

RiscVLinuxUserEmulationModel::RiscVLinuxUserEmulationModel() : LinuxUserEmulationModel("risc-v") { }

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

	memory::guest_addr_t kernel_helper_region = 0xffff0000;
	GetMemoryModel().GetMappingManager()->MapRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[eabi]");

	/* random data */
	GetMemoryModel().Write32(0xffff0000, 0xbabecafe);
	GetMemoryModel().Write32(0xffff0004, 0xdeadbabe);
	GetMemoryModel().Write32(0xffff0008, 0xfeedc0de);
	GetMemoryModel().Write32(0xffff000c, 0xcafedead);

	return true;
}

gensim::DecodeContext* RiscVLinuxUserEmulationModel::GetNewDecodeContext(gensim::Processor& cpu)
{
	return new arch::riscv::RiscVDecodeContext(&cpu);
}



bool RiscVLinuxUserEmulationModel::InvokeSignal(int signum, uint32_t next_pc, SignalData* data)
{
	assert(false);

	//TODO

	return false;
}

archsim::abi::ExceptionAction RiscVLinuxUserEmulationModel::HandleException(archsim::ThreadInstance *cpu, unsigned int category, unsigned int data)
{
	UNIMPLEMENTED;
//	
//	if(category == 1024) {
//		GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, (void*)(uint64_t)0);
//		return archsim::abi::ResumeNext;
//	}
//
//	if(category == 0) {
//		gensim::RegisterBankDescriptor& bank = cpu.GetRegisterBankDescriptor("GPR");
//		uint32_t* registers = (uint32_t*)bank.GetBankDataStart();
//
//		archsim::abi::SyscallRequest request {0, cpu};
//		request.syscall = registers[17];
//
//		archsim::abi::SyscallResponse response;
//		response.action = ResumeNext;
//
//		request.arg0 = registers[10];
//		request.arg1 = registers[11];
//		request.arg2 = registers[12];
//		request.arg3 = registers[13];
//		request.arg4 = registers[14];
//		request.arg5 = registers[15];
//
//		if(EmulateSyscall(request, response)) {
//			registers[10] = response.result;
//		} else {
//			LC_ERROR(LogEmulationModelRiscVLinux) << "Syscall not supported: " << std::hex << "0x" << request.syscall << "(" << std::dec << request.syscall << ")";
//			registers[0] = -1;
//		}
//
//		return response.action;
//	}
//
//	return AbortSimulation;
}
