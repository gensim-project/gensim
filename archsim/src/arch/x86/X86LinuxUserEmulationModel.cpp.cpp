/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * arch/arm/ArmLinuxUserEmulationModel.cpp
 */
#include "arch/x86/X86LinuxUserEmulationModel.h"
#include "arch/x86/X86DecodeContext.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"
#include "system.h"

#include <sys/auxv.h>
#include <sys/mman.h>

#ifdef CONFIG_GFX
#ifdef CONFIG_SDL
#include "abi/devices/gfx/SDLScreen.h"
#endif
#include "abi/devices/generic/DoomDevice.h"
#endif

using namespace archsim::abi;
using namespace archsim::abi::memory;
using namespace archsim::arch::x86;

UseLogContext(LogEmulationModelUser);
DeclareChildLogContext(LogEmulationModelX86Linux, LogEmulationModelUser, "X86-Linux");

RegisterComponent(archsim::abi::EmulationModel, X86LinuxUserEmulationModel, "x86-user", "X86 Linux user emulation model");

X86LinuxUserEmulationModel::X86LinuxUserEmulationModel() : LinuxUserEmulationModel("x86", true, AuxVectorEntries("x86_64", 0xbfebfbff, 0)) { }

X86LinuxUserEmulationModel::~X86LinuxUserEmulationModel() { }

bool X86LinuxUserEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!LinuxUserEmulationModel::Initialise(system, uarch))
		return false;

	return true;
}

void X86LinuxUserEmulationModel::Destroy()
{
	LinuxUserEmulationModel::Destroy();
}

extern char x86_linux_vdso_start, x86_linux_vdso_end;
extern uint32_t x86_linux_vdso_size;


bool X86LinuxUserEmulationModel::PrepareBoot(System& system)
{
	if (!LinuxUserEmulationModel::PrepareBoot(system)) {
		LC_ERROR(LogEmulationModelX86Linux) << "Failed to init linux user system";
		return false;
	}

	LC_DEBUG1(LogEmulationModelX86Linux) << "Initialising X86 Kernel Helpers";

	memory::guest_addr_t kernel_helper_region = 0xffff0000_ga;
	GetMemoryModel().GetMappingManager()->MapRegion(kernel_helper_region, 0x4000, (memory::RegionFlags)(memory::RegFlagRead | memory::RegFlagWrite), "[eabi]");

	/* random data */
	GetMemoryModel().Write32(0xffff0000_ga, 0xbabecafe);
	GetMemoryModel().Write32(0xffff0004_ga, 0xdeadbabe);
	GetMemoryModel().Write32(0xffff0008_ga, 0xfeedc0de);
	GetMemoryModel().Write32(0xffff000c_ga, 0xcafedead);

	/* Copy host vdso to guest */
	vdso_ptr_ = 0x7fff00000000_ga;
	LC_DEBUG1(LogEmulationModelX86Linux) << "Writing VDSO into guest memory (" << x86_linux_vdso_size << " bytes)";
	GetMemoryModel().GetMappingManager()->MapRegion(vdso_ptr_, x86_linux_vdso_size, RegFlagReadWriteExecute, "vdso");
	GetMemoryModel().Write(vdso_ptr_, (uint8_t*)&x86_linux_vdso_start, x86_linux_vdso_size);

	return true;
}

gensim::DecodeContext* X86LinuxUserEmulationModel::GetNewDecodeContext(archsim::core::thread::ThreadInstance& cpu)
{
	gensim::DecodeContext *ctx = new arch::x86::X86DecodeContext(cpu.GetArch());
	return ctx;
}

bool X86LinuxUserEmulationModel::InvokeSignal(int signum, uint32_t next_pc, SignalData* data)
{
	assert(false);

	//TODO

	return false;
}

archsim::abi::ExceptionAction X86LinuxUserEmulationModel::HandleException(archsim::core::thread::ThreadInstance *cpu, unsigned int category, unsigned int data)
{
	if(category == 1024) {
		GetSystem().GetPubSub().Publish(PubSubType::L1ICacheFlush, (void*)(uint64_t)0);
		return archsim::abi::ResumeNext;
	}
	uint64_t* registers = (uint64_t*)cpu->GetRegisterFileInterface().GetData();

	if(category == 0) {
		LC_DEBUG1(LogEmulationModelX86Linux) << "Syscall at " << Address(cpu->GetPC());
		archsim::abi::SyscallRequest request {0, cpu, 0, 0, 0, 0, 0, 0};
		request.syscall = registers[0];

		archsim::abi::SyscallResponse response;
		response.action = ResumeNext;

		request.arg0 = registers[7];
		request.arg1 = registers[6];
		request.arg2 = registers[2];
		request.arg3 = registers[10];
		request.arg4 = registers[8];
		request.arg5 = registers[9];

		if(EmulateSyscall(request, response)) {
			registers[0] = (int64_t)response.result;
		} else {
			LC_ERROR(LogEmulationModelX86Linux) << "Syscall not supported: " << std::hex << "0x" << request.syscall << "(" << std::dec << request.syscall << ")";
			registers[0] = -1;
		}

		// xxx arm hax
		// currently a syscall cannot return an action other than resume, so
		// we need to exit manually here.
		if(request.syscall == 93) {
			cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
			return AbortSimulation;
		}

		return response.action;
	} else if(category == 1) {
		// support call
		switch(data) {
			case 0: { // rdtsc
				asm volatile ("rdtsc" : "=a"(registers[0]), "=d"(registers[2]));
				return ResumeNext;
			}
			case 1: { // clflush
				GetSystem().GetPubSub().Publish(PubSubType::FlushAllTranslations, nullptr);
				return ResumeNext;
			}
			case 2: {
				uint32_t eax = registers[0];

				registers[0] = 0;
				registers[1] = 0;
				registers[2] = 0;
				registers[3] = 0;

				switch(eax) {


					case 0: {
						// EBX “Genu” ECX “ntel” EDX “ineI”
						uint32_t ebx = 'G' << 0 | 'e' << 8 | 'n' << 16 | 'u' << 24;
						uint32_t ecx = 'n' << 0 | 't' << 8 | 'e' << 16 | 'l' << 24;
						uint32_t edx = 'i' << 0 | 'n' << 8 | 'e' << 16 | 'I' << 24;
						registers[1] = ecx;
						registers[2] = edx;
						registers[3] = ebx;

						registers[0] = 0x16;
						break;
					}
					case 1: {
						registers[0] = 0x000506e3;

						uint32_t ecx = 0;
						ecx |= (0 << 0); // SSE3
						ecx |= (0 << 1); // PCLMULQDQ
						ecx |= (0 << 2); // 64 bit debug store
						ecx |= (0 << 3); // MONITOR and MWAIT
						ecx |= (0 << 4); // CPL qualified debug store
						ecx |= (0 << 5); // VMX
						ecx |= (0 << 6); // SMX
						ecx |= (0 << 7); // EST
						ecx |= (0 << 8); // TM2
						ecx |= (0 << 9); // SSSE3
						ecx |= (0 << 10); // CNXT-ID
						ecx |= (0 << 11); // Silicon Debug Interface
						ecx |= (0 << 12); // FMA3
						ecx |= (0 << 13); // CMPXCHG16B
						ecx |= (0 << 14); // XTPR
						ecx |= (0 << 15); // PDCM
						ecx |= (0 << 16); // RESERVED
						ecx |= (0 << 17); // PCID
						ecx |= (0 << 18); // DCA
						ecx |= (0 << 19); // SSE4.1
						ecx |= (0 << 20); // SSE4.2
						ecx |= (0 << 21); // X2APIC
						ecx |= (0 << 22); // MOVBE
						ecx |= (1 << 23); // POPCNT
						ecx |= (0 << 24); // TSC-DEADLINE
						ecx |= (0 << 25); // AES
						ecx |= (1 << 26); // XSAVE
						ecx |= (0 << 27); // OSXSAVE
						ecx |= (0 << 28); // AVX
						ecx |= (0 << 29); // F16C
						ecx |= (0 << 30); // RDRAND
						ecx |= (0 << 31); // HYPERVISOR

						uint32_t edx = 0;
						edx |= (1 << 0); // FPU
						edx |= (0 << 1); // VME
						edx |= (0 << 2); // DE
						edx |= (0 << 3); // PSE
						edx |= (0 << 4); // TSC
						edx |= (0 << 5); // MSR
						edx |= (0 << 6); // PAE
						edx |= (0 << 7); // MCE
						edx |= (1 << 8); // CX8
						edx |= (0 << 9); // APIC
						edx |= (0 << 10); // RESERVED
						edx |= (0 << 11); // SEP
						edx |= (0 << 12); // MTRR
						edx |= (0 << 13); // PGE
						edx |= (0 << 14); // MCA
						edx |= (1 << 15); // CMOV
						edx |= (0 << 16); // PAT
						edx |= (0 << 17); // PSE-36
						edx |= (0 << 18); // PSN
						edx |= (1 << 19); // CFSH
						edx |= (0 << 20); // RESERVED
						edx |= (0 << 21); // DS
						edx |= (0 << 22); // ACPI
						edx |= (1 << 23); // MMX
						edx |= (0 << 24); // FXSR
						edx |= (1 << 25); // SSE
						edx |= (1 << 26); // SSE2
						edx |= (0 << 27); // SS
						edx |= (0 << 28); // HTT
						edx |= (0 << 29); // TM
						edx |= (0 << 30); // IA64
						edx |= (0 << 31); // PBE

						registers[1] = ecx;
						registers[2] = edx; //edx;
						registers[3] = 0; //ebx;

						break;
					}

					case 2: {
						registers[0] = 0x01060a01;
						registers[1] = 0;
						registers[2] = 0;
						registers[3] = 0;
						break;
					}

					case 7: {
						registers[0] = 0;

						registers[1] = 0; // ecx
						registers[2] = 0xc000000; //edx
						registers[3] = 0; //ebx

						break;
					}
					case 0xd: {
						registers[0] = 0x480;
						registers[1] = 0;
						registers[2] = 0;
						registers[3] = 0x440;

						break;
					}
					default: {
						LC_ERROR(LogEmulationModelX86Linux) << "Unknown CPUID leaf: " << std::hex << eax;
						break;
					}
				}

				return ResumeNext;
			}
		}

	}

	cpu->SendMessage(archsim::core::thread::ThreadMessage::Halt);
	return AbortSimulation;
}
