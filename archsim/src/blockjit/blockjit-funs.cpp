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
#include "util/LogContext.h"

#include "translate/profile/Region.h"

#include "translate/jit_funs.h"

DeclareChildLogContext(LogBlockJitFuns, LogCPU, "BlockJITFuns");
UseLogContext(LogBlockJitFuns)

extern "C" {
	void blkProfile(gensim::Processor *cpu, void *region, uint32_t address)
	{
		archsim::translate::profile::Region *rgn = (archsim::translate::profile::Region *)region;
		rgn->TraceBlock(*cpu, address);
	}

	uint8_t blkRead8(gensim::Processor **pcpu, uint32_t address)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkRead8";
		uint8_t data;
		gensim::Processor *cpu = *pcpu;
		auto rval = cpu->GetMemoryModel().Read8(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead8(cpu, address, data);

		if(UNLIKELY(rval)) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return data;
	}

	uint16_t blkRead16(gensim::Processor **pcpu, uint32_t address)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkRead16";
		uint16_t data;
		gensim::Processor *cpu = *pcpu;
		auto rval = cpu->GetMemoryModel().Read16(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead16(cpu, address, data);

		if(UNLIKELY(rval)) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return data;
	}

	uint32_t blkRead32(gensim::Processor **pcpu, uint32_t address)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkRead32";
		uint32_t data;
		gensim::Processor *cpu = *pcpu;
		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read32(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead32(cpu, address, data);

		if(UNLIKELY(rval)) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return data;
	}

	uint8_t blkRead8User(gensim::Processor **pcpu, uint32_t address)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkRead8User";
		uint32_t data;
		gensim::Processor *cpu = *pcpu;
		auto rval = cpu->GetMemoryModel().Read8User(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead8(cpu, address, data);

		if(UNLIKELY(rval)) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return data;
	}

	uint16_t blkRead16User(gensim::Processor **pcpu, uint32_t address)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkRead16User";
		__builtin_unreachable();
		return 0;
	}

	uint32_t blkRead32User(gensim::Processor **pcpu, uint32_t address)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkRead32User";
		uint32_t data;
		gensim::Processor *cpu = *pcpu;
		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read32User(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead32(cpu, address, data);

		if(UNLIKELY(rval)) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return data;
	}

	void blkWrite8(gensim::Processor **pcpu, uint32_t address, uint32_t value)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkWrite8";
		gensim::Processor *cpu = *pcpu;
		auto rval = cpu->GetMemoryModel().Write8(address, value);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemWrite8(cpu, address, value);

		if(UNLIKELY(rval)) {
			if(rval < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
//				cpuReturnToSafepoint(cpu);
			}
		}
	}

	void blkWrite16(gensim::Processor **pcpu, uint32_t address, uint32_t value)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkWrite16";
		gensim::Processor *cpu = *pcpu;
		auto rval = cpu->GetMemoryModel().Write16(address, value);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemWrite16(cpu, address, value);

		if(UNLIKELY(rval)) {
			if(rval < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
//				cpuReturnToSafepoint(cpu);
			}
		}
	}

	void blkWrite32(gensim::Processor **pcpu, uint32_t address, uint32_t value)
	{
		LC_DEBUG2(LogBlockJitFuns) << "blkWrite32";
		gensim::Processor *cpu = *pcpu;
		archsim::abi::memory::FunctionBasedSystemMemoryModel::write_handler_t fun = ((archsim::abi::memory::FunctionBasedSystemMemoryModel::write_handler_t*)cpu->state.smm_write_cache)[address>>12];

		auto rval = fun((cpuState*)pcpu, address);
		if(rval.fault) {
			cpuWrite32(cpu, address, value);
			return;
		}
		*((uint32_t*)rval.ptr) = value;
	}
}
