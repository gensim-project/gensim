/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * arch/arm/ArmLinuxUserEmulationModel.cpp
 */
#include "arch/arm/ArmLinuxUserEmulationModel.h"
#include "arch/arm/ARMDecodeContext.h"

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
using namespace archsim::arch::arm;

UseLogContext(LogEmulationModelUser);
DeclareChildLogContext(LogEmulationModelArmLinux, LogEmulationModelUser, "ARM-Linux");

RegisterComponent(archsim::abi::EmulationModel, ArmLinuxUserEmulationModel, "arm-user", "ARM Linux user emulation model");

ArmLinuxUserEmulationModel::ArmLinuxUserEmulationModel() : ArmLinuxUserEmulationModel(archsim::options::ArmOabi ? ABI_OABI : ABI_EABI) { }

ArmLinuxUserEmulationModel::ArmLinuxUserEmulationModel(ArmLinuxABIVersion version) : LinuxUserEmulationModel("arm"), abi_version(version) { }

ArmLinuxUserEmulationModel::~ArmLinuxUserEmulationModel() { }

bool ArmLinuxUserEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!LinuxUserEmulationModel::Initialise(system, uarch))
		return false;

	return InstallKernelHelpers();
}

void ArmLinuxUserEmulationModel::Destroy()
{
	LinuxUserEmulationModel::Destroy();
}

bool ArmLinuxUserEmulationModel::PrepareBoot(System& system)
{
	if (!LinuxUserEmulationModel::PrepareBoot(system)) {
		return false;
	}

	GetMainThread()->GetFeatures().SetFeatureLevel("ARM_FPU_ENABLED_FPEXC", 1);
	GetMainThread()->GetFeatures().SetFeatureLevel("ARM_FPU_ENABLED_CPACR", 2);
	GetMainThread()->GetFeatures().SetFeatureLevel("ARM_NEON_ENABLED_CPACR", 1);

	*GetMainThread()->GetRegisterFileInterface().GetEntry<uint32_t>("FPEXC") = 0x40000000;

#ifdef CONFIG_GFX
	if (archsim::options::Doom) {
		fprintf(stderr, "Installing doomvice\n");

		GetMemoryModel().GetMappingManager()->MapRegion(0xfbff0000, 0x100000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "screen");

		archsim::abi::devices::gfx::VirtualScreenManagerBase *screen_man = NULL;
		if(!GetComponentInstance(archsim::options::ScreenManagerType, screen_man)) {
			fprintf(stderr, "Could not instantiate Screen Manager %s!\n%s\n", archsim::options::ScreenManagerType.GetValue().c_str(), GetRegisteredComponents(screen_man).c_str());;
			return false;
		}

		auto doom = new devices::generic::DoomDevice(*screen_man);
		GetBootCore()->peripherals.RegisterDevice("doom", doom);
		GetBootCore()->peripherals.AttachDevice("doom", 13);
	}
#endif

//	GetBootCore()->peripherals.InitialiseDevices();

	return true;
}

gensim::DecodeContext* ArmLinuxUserEmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance &cpu)
{
	return new archsim::arch::arm::ARMDecodeContext(cpu.GetArch());
}


bool ArmLinuxUserEmulationModel::InvokeSignal(int signum, uint32_t next_pc, SignalData* data)
{
	LC_DEBUG1(LogEmulationModelArmLinux) << "Invoking Signal " << signum << ", Next PC = " << std::hex << next_pc << ", Target PC = " << std::hex << data->pc;

	// Store the target PC, i.e. the address of the signal handler
	GetMemoryModel().Poke(0xffff201c, (uint8_t *)&data->pc, 4);

	// Store the next PC after the signal handler to be executed
	GetMemoryModel().Poke(0xffff205c, (uint8_t *)&next_pc, 4);

	// Write the PC of the signal dispatch helper into the CPU
	GetMainThread()->SetPC(Address(0xffff2000));

	return false;
}

bool ArmLinuxUserEmulationModel::InstallKernelHelpers()
{
	LC_DEBUG1(LogEmulationModelArmLinux) << "Initialising ARM Kernel Helpers";

	memory::guest_addr_t kernel_helper_region = 0xffff0000;
	GetMemoryModel().GetMappingManager()->MapRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[eabi]");

	// TODO: (1) Improve this code sequence by unlocking the whole page, and writing it in a one'er.
	//       (2) Get ACTUAL random data from /dev/urandom
	//       (3) Load machine code from external file, to increase portability (e.g. use xcc to generate)

	/* random data */
	GetMemoryModel().Write32(0xffff0000, 0xbabecafe);
	GetMemoryModel().Write32(0xffff0004, 0xdeadbabe);
	GetMemoryModel().Write32(0xffff0008, 0xfeedc0de);
	GetMemoryModel().Write32(0xffff000c, 0xcafedead);

	/* cmpxchg64 */
	GetMemoryModel().Write32(0xffff0f60, 0xe92d4070);
	GetMemoryModel().Write32(0xffff0f64, 0xe8900030);
	GetMemoryModel().Write32(0xffff0f68, 0xe8914040);
	GetMemoryModel().Write32(0xffff0f6c, 0xe8920003);
	GetMemoryModel().Write32(0xffff0f70, 0xe0303004);
	GetMemoryModel().Write32(0xffff0f74, 0x00313005);
	GetMemoryModel().Write32(0xffff0f78, 0x08824040);
	GetMemoryModel().Write32(0xffff0f7c, 0xe2730000);
	GetMemoryModel().Write32(0xffff0f80, 0xe8bd8070);
	AddSymbol(0xffff0f60, 36, "__aeabi_cmpxchg64", FunctionSymbol);

	GetMemoryModel().Write32(0xffff0fa0, 0xe12fff1e); /* memory_barrier */
	AddSymbol(0xffff0fa0, 4, "__aeabi_memory_barrier", FunctionSymbol);

	/* cmpxchg */
	GetMemoryModel().Write32(0xffff0fc0, 0xe5923000);
	GetMemoryModel().Write32(0xffff0fc4, 0xe0533000);
	GetMemoryModel().Write32(0xffff0fc8, 0x05821000);
	GetMemoryModel().Write32(0xffff0fcc, 0xe2730000);
	GetMemoryModel().Write32(0xffff0fd0, 0xe12fff1e);
	AddSymbol(0xffff0fc0, 20, "__aeabi_cmpxchg", FunctionSymbol);

	/* get_tls */
	GetMemoryModel().Write32(0xffff0fe0, 0xe59f0008);
	GetMemoryModel().Write32(0xffff0fe4, 0xe12fff1e);
	AddSymbol(0xffff0fe0, 8, "__aeabi_get_tls", FunctionSymbol);

	/* version */
	GetMemoryModel().Write32(0xffff1000, 4);

	/* signal handling */
	GetMemoryModel().Write32(0xffff2000, 0xe58f0018);	// str r0, [pc, #24]		// Save r0
	GetMemoryModel().Write32(0xffff2004, 0xe28f0018);	// add r0, pc, #24		// Utilise r0 to point to the reg save area
	GetMemoryModel().Write32(0xffff2008, 0xe8807ffe);	// stm r0, {r1-r14}		// Store all registers into the save area
	GetMemoryModel().Write32(0xffff200c, 0xe59f0008);	// ldr r0, [pc, #8]		// Load the address of the signal handler in to r0
	GetMemoryModel().Write32(0xffff2010, 0xe12fff30);	// blx r0				// Branch to the signal handler
	GetMemoryModel().Write32(0xffff2014, 0xe28f0004);	// add r0, pc, #4		// Load the address of the reg save area into r0
	GetMemoryModel().Write32(0xffff2018, 0xe890ffff);	// ldm r0, {r0-r15}		// Restore all registers (including the PC)

	GetMemoryModel().GetMappingManager()->ProtectRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite | memory::RegFlagExecute));

	return true;
}

archsim::abi::ExceptionAction ArmLinuxUserEmulationModel::HandleException(archsim::core::thread::ThreadInstance* thread, unsigned int category, unsigned int data)
{
	if (category == 3) {
		auto bank = thread->GetRegisterFileInterface().GetEntry<uint32_t>("RB");
		uint32_t* registers = (uint32_t*)bank;

		archsim::abi::SyscallRequest request {0, thread};
		if(IsOABI()) {
			request.syscall = data & 0xfffff;
		} else if(IsEABI()) {
			request.syscall = registers[7];
		} else {
			assert(!"I don't know how to handle this kind of syscall!");
		}
		archsim::abi::SyscallResponse response;
		response.action = ResumeNext;

		request.arg0 = registers[0];
		request.arg1 = registers[1];
		request.arg2 = registers[2];
		request.arg3 = registers[3];
		request.arg4 = registers[4];
		request.arg5 = registers[5];

		if (EmulateSyscall(request, response)) {
			registers[0] = response.result;
		} else {
			LC_WARNING(LogEmulationModelArmLinux) << "Syscall not supported: " << std::hex << request.syscall;
			registers[0] = -1;
		}

		// xxx arm hax
		// currently a syscall cannot return an action other than resume, so
		// we need to exit manually here.
		if(request.syscall == 0x1) {
			thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
			return AbortSimulation;
		}

		return response.action;

	} else if (category == 11) {
		LC_ERROR(LogEmulationModelArmLinux) << "Undefined Instruction Exception @ " << std::hex << thread->GetPC();
		GetSystem().exit_code = 1;
		thread->SendMessage(archsim::core::thread::ThreadMessage::Halt);
		return AbortSimulation;
	} else {
		LC_ERROR(LogEmulationModelArmLinux) << "Unsupported Exception " << category << ":" << data;
		GetSystem().exit_code = 1;
		return AbortSimulation;
	}

	return ResumeNext;
}
