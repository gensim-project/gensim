/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * AngelSyscallHandler.h
 *
 *  Created on: 8 Sep 2014
 *      Author: harry
 */

#ifndef ANGELSYSCALLHANDLER_H_
#define ANGELSYSCALLHANDLER_H_

#include <stdint.h>

namespace gensim
{
	class Processor;
}

namespace archsim
{

	namespace abi
	{
		namespace memory
		{
			class MemoryModel;
		}
	}

	namespace arch
	{
		namespace arm
		{

			class AngelSyscallHandler
			{
			public:
				AngelSyscallHandler(archsim::abi::memory::MemoryModel &mem, uint32_t heap_base, uint32_t heap_limit, uint32_t stack_base, uint32_t stack_limit);
				bool HandleSyscall(gensim::Processor &cpu);

			private:
				bool HandleOpen(uint32_t arg, uint32_t &out);
				bool HandleClose(uint32_t arg, uint32_t &out);
				bool HandleWriteC(uint32_t arg, uint32_t &out);
				bool HandleWrite0(uint32_t arg, uint32_t &out);
				bool HandleWrite(uint32_t arg, uint32_t &out);
				bool HandleRead(uint32_t arg, uint32_t &out);
				bool HandleSeek(uint32_t arg, uint32_t &out);
				bool HandleFlen(uint32_t arg, uint32_t &out);
				bool HandleClock(uint32_t arg, uint32_t &out);
				bool HandleSystem(uint32_t arg, uint32_t &out);
				bool HandleIsTty(uint32_t arg, uint32_t &out);
				bool HandleErrno(uint32_t arg, uint32_t &out);
				bool HandleGetCmdline(uint32_t arg, uint32_t &out);
				bool HandleHeapInfo(uint32_t arg, uint32_t &out);
				bool HandleKill(uint32_t arg, uint32_t &out);


				archsim::abi::memory::MemoryModel &memory_model;
				uint32_t heap_base, heap_limit, stack_base, stack_limit;
			};

		}
	}
}




#endif /* ANGELSYSCALLHANDLER_H_ */
