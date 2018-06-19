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
			UserEmulationModel(const user::arch_descriptor_t &arch);
			virtual ~UserEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

			gensim::Processor* GetCore(int id);
			
			archsim::core::thread::ThreadInstance *GetMainThread();
			
			void ResetCores();
			void HaltCores();

			bool PrepareBoot(System& system);

			bool EmulateSyscall(SyscallRequest& request, SyscallResponse& response);
			
			Address MapAnonymousRegion(size_t size, archsim::abi::memory::RegionFlags flags);
			bool MapRegion(Address addr, size_t size, archsim::abi::memory::RegionFlags flags, const std::string &region_name);

			void SetInitialBreak(unsigned int brk);
			void SetBreak(unsigned int brk);
			unsigned int GetBreak();
			unsigned int GetInitialBreak();

			bool AssertSignal(int signum, SignalData* data);
			virtual bool InvokeSignal(int signum, uint32_t next_pc, SignalData* data);

			virtual ExceptionAction HandleException(archsim::core::thread::ThreadInstance* cpu, unsigned int category, unsigned int data) override;
			void PrintStatistics(std::ostream& stream);

		private:
			bool PrepareStack(System& system, loader::UserElfBinaryLoader& elf_loader);
			bool InitialiseProgramArguments();
			
			user::SyscallHandler &syscall_handler_;

			int global_argc, global_envc;
			char** global_argv;

			unsigned int _initial_entry_point;
			unsigned int _initial_stack_pointer;
			unsigned int _stack_size;
			unsigned int _initial_program_break;
			unsigned int _program_break;
			
			archsim::core::thread::ThreadInstance *main_thread_;
		};
	}
}

#endif	/* USEREMULATIONMODEL_H */

