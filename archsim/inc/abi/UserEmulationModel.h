/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   UserEmulationModel.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 14:48
 */

#ifndef USEREMULATIONMODEL_H
#define	USEREMULATIONMODEL_H

#include "abi/EmulationModel.h"
#include "abi/memory/MemoryModel.h"
#include "abi/user/SyscallHandler.h"
#include "core/thread/ThreadInstance.h"
#include "abi/loader/BinaryLoader.h"

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class MemoryModel;
		}

		struct SyscallRequest {
			unsigned int syscall;

			archsim::core::thread::ThreadInstance *thread;

			unsigned int arg0;
			unsigned int arg1;
			unsigned int arg2;
			unsigned int arg3;
			unsigned int arg4;
			unsigned int arg5;
		};

		struct SyscallResponse {
			unsigned int result;
			ExceptionAction action;
		};

		class UserEmulationModel : public EmulationModel
		{
		public:
			UserEmulationModel(const user::arch_descriptor_t &arch, bool is_64bit_binary);
			virtual ~UserEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

			gensim::Processor* GetCore(int id);

			archsim::core::thread::ThreadInstance *GetMainThread();

			void ResetCores();
			void HaltCores() override;

			bool PrepareBoot(System& system) override;

			bool EmulateSyscall(SyscallRequest& request, SyscallResponse& response);

			Address MapAnonymousRegion(size_t size, archsim::abi::memory::RegionFlags flags);
			bool MapRegion(Address addr, size_t size, archsim::abi::memory::RegionFlags flags, const std::string &region_name);

			void SetInitialBreak(Address brk);
			void SetBreak(Address brk);
			Address GetBreak();
			Address GetInitialBreak();

			bool AssertSignal(int signum, SignalData* data) override;
			virtual bool InvokeSignal(int signum, uint32_t next_pc, SignalData* data) override;

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance* cpu, unsigned int category, unsigned int data) override;
			void PrintStatistics(std::ostream& stream) override;

			bool Is64BitBinary() const { return is_64bit_; }
		private:
			bool PrepareStack(System &system, Address elf_phdr_location, uint32_t elf_phnum, uint32_t elf_phentsize);
			bool InitialiseProgramArguments();

			bool is_64bit_;
			
			user::SyscallHandler &syscall_handler_;

			int global_argc, global_envc;
			char** global_argv;

			Address _initial_entry_point;
			Address _initial_stack_pointer;
			Address _initial_program_break;
			Address _program_break;
			unsigned int _stack_size;

			archsim::core::thread::ThreadInstance *main_thread_;
		};
	}
}

#endif	/* USEREMULATIONMODEL_H */

