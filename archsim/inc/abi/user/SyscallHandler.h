/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SyscallHandler.h
 * Author: s0457958
 *
 * Created on 19 November 2013, 14:48
 */

#ifndef SYSCALLHANDLER_H
#define SYSCALLHANDLER_H

#include <map>
#include <string>

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace core {
		namespace thread {
			class ThreadInstance;
		}
	}
	namespace abi
	{
		struct SyscallRequest;
		struct SyscallResponse;

		namespace user
		{

			typedef unsigned int (*SYSCALL_FN_GENERIC)(archsim::core::thread::ThreadInstance*, unsigned int...);
			typedef std::string arch_descriptor_t;

			class SyscallHandler;

			class SyscallHandlerProvider
			{
			public:
				SyscallHandler &Get(const arch_descriptor_t &arch);

				static SyscallHandlerProvider &Singleton();
			private:
				std::map<arch_descriptor_t, SyscallHandler> handler_map_;
				static SyscallHandlerProvider *singleton_;
			};

			class SyscallHandler
			{
			public:
				SyscallHandler();
				virtual ~SyscallHandler();

				bool HandleSyscall(SyscallRequest& request, SyscallResponse& response) const;

				void RegisterSyscall(unsigned int nr, SYSCALL_FN_GENERIC fn, const std::string& name);

			private:
				std::map<unsigned int, SYSCALL_FN_GENERIC> syscall_fns;
				std::map<unsigned int, std::string> syscall_fn_names;
			};

			class SyscallRegistration
			{
			public:
				SyscallRegistration(const std::string &arch_name, unsigned int nr, SYSCALL_FN_GENERIC fn, const std::string& name)
				{
					SyscallHandlerProvider::Singleton().Get(arch_name).RegisterSyscall(nr, fn, name);
				};
			};

#define DEFINE_SYSCALL(arch, nr, fn, dbg) archsim::abi::user::SyscallRegistration __syscall_##fn##_##nr(arch, nr, (archsim::abi::user::SYSCALL_FN_GENERIC)fn, dbg)
		}
	}
}

#endif /* SYSCALLHANDLER_H */
