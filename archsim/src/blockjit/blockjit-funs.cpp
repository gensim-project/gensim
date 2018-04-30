/*
 * blockjit-funs.cpp
 *
 *  Created on: 22 Oct 2015
 *      Author: harry
 */

#include "define.h"

#include "abi/devices/MMU.h"
#include "abi/memory/MemoryModel.h"
#include "abi/memory/system/FunctionBasedSystemMemoryModel.h"
#include "gensim/gensim_processor_state.h"
#include "gensim/gensim_processor.h"
#include "core/MemoryInterface.h"
#include "util/LogContext.h"

#include "translate/profile/Region.h"

#include "translate/jit_funs.h"

DeclareChildLogContext(LogBlockJitFuns, LogCPU, "BlockJITFuns");
UseLogContext(LogBlockJitFuns)

using archsim::Address;

extern "C" {
	void blkProfile(gensim::Processor *cpu, void *region, uint32_t address)
	{
		archsim::translate::profile::Region *rgn = (archsim::translate::profile::Region *)region;
		rgn->TraceBlock(*cpu, address);
	}

	uint8_t blkRead8(archsim::core::thread::ThreadInstance **thread_p, uint32_t address, uint32_t interface)
	{
		auto *thread = *thread_p;
		uint8_t data;
		auto rval = thread->GetMemoryInterface(interface).Read8(Address(address), data);

		if(rval != archsim::MemoryResult::OK) {
			thread->TakeMemoryException(thread->GetMemoryInterface(interface), Address(address));
		}

		return data;
	}

	uint16_t blkRead16(archsim::core::thread::ThreadInstance **thread_p, uint32_t address, uint32_t interface)
	{
		auto *thread = *thread_p;
		uint16_t data;
		auto rval = thread->GetMemoryInterface(interface).Read16(Address(address), data);

		if(rval != archsim::MemoryResult::OK) {
			thread->TakeMemoryException(thread->GetMemoryInterface(interface), Address(address));
		}

		return data;
	}

	uint32_t blkRead32(archsim::core::thread::ThreadInstance **thread_p, uint32_t address, uint32_t interface)
	{
		auto *thread = *thread_p;
		uint32_t data;
		auto rval = thread->GetMemoryInterface(interface).Read32(Address(address), data);

		if(rval != archsim::MemoryResult::OK) {
			thread->TakeMemoryException(thread->GetMemoryInterface(interface), Address(address));
		}

		return data;
	}

}
