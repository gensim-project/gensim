//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2009 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
// uint32_t
//  This file implements an API to the ISS using C linkage for JIT simulation.
//
// =====================================================================

#include "define.h"

#include "abi/devices/MMU.h"
#include "abi/memory/MemoryModel.h"
#include "gensim/gensim_processor_state.h"
#include "gensim/gensim_processor.h"

#include "tracing/TraceManager.h"

#include "util/LogContext.h"

#include "translate/jit_funs.h"

#include <cfenv>

DeclareChildLogContext(LogJitFuns, LogCPU, "LogJITFuns");
UseLogContext(LogJitFuns)
extern "C" {
	void cpuSetFeature(gensim::Processor *cpu, uint32_t feature, uint32_t level)
	{
		cpu->GetFeatures().SetFeatureLevel(feature, level);
	}

	uint32_t cpuGetRoundingMode(gensim::Processor *cpu)
	{
		uint32_t rounding_mode = fegetround();
		LC_DEBUG1(LogJitFuns) << "Got host rounding mode " << rounding_mode;
		switch(rounding_mode) {
			case FE_TONEAREST:
				return 0;
			case FE_UPWARD:
				return 1;
			case FE_DOWNWARD:
				return 2;
			case FE_TOWARDZERO:
				return 3;
		}

		//Unknown mode!
		LC_ERROR(LogJitFuns) << "Unknown host rounding mode " << rounding_mode << "!";
		return 0;
	}
	void cpuSetRoundingMode(gensim::Processor *cpu, uint32_t mode)
	{
		LC_DEBUG1(LogJitFuns) << "Set rounding mode to mode " << mode;
		uint32_t rounding_mode = 0;
		switch(mode) {
			case 0:
				rounding_mode = FE_TONEAREST;
				break;
			case 1:
				rounding_mode = FE_UPWARD;
				break;
			case 2:
				rounding_mode = FE_DOWNWARD;
				break;
			case 3:
				rounding_mode = FE_TOWARDZERO;
				break;
			default:
				LC_ERROR(LogJitFuns) << "Unknown guest rounding mode " << mode << "!";
				break;
		}
		fesetround(rounding_mode);
	}

	uint32_t cpuGetFlushMode(gensim::Processor *cpu)
	{
		uint32_t mxcsr;
		asm volatile("stmxcsr %0" : "=m"(mxcsr));
		return (mxcsr >> 6) & 1;
	}
	void cpuSetFlushMode(gensim::Processor *cpu, uint32_t mode)
	{
		uint32_t mxcsr;
		uint32_t flags = (1 << 6 | 1 << 11 | 1 << 15); // DAZ, UE, FTZ
		
		asm volatile ("stmxcsr %0" : "=m"(mxcsr));
		if(mode) {
			mxcsr |= flags;
		} else {
			mxcsr &= ~flags;
		}
		asm volatile ("ldmxcsr %0" :: "m"(mxcsr));
	}

	uint8_t cpuGetExecMode(gensim::Processor *cpu)
	{
		return cpu->interrupt_mode;
	}

	uint32_t jitChecksumPage(gensim::Processor *cpu, uint32_t page_base)
	{
		uint32_t checksum = 0;
		host_addr_t page_data;

		if(cpu->GetEmulationModel().GetMemoryModel().LockRegion(page_base, 4096, page_data)) {
			uint32_t *page_words = (uint32_t*)page_data;
			for(uint32_t i = 0; i < 1024; ++i) checksum += page_words[i];
		}

		return checksum;
	}

	void jitCheckChecksum(uint32_t expected, uint32_t actual)
	{
		if(expected != actual) {
			fprintf(stderr, "CHECKSUM FAILURE! %x != %x\n", expected, actual);
			assert(expected == actual);
		}
	}

	int jitDebug0()
	{
		fprintf(stderr, "debug()\n");
		return 0;
	}

	int jitDebug1(void *a)
	{
		fprintf(stderr, "debug(%p)\n", a);
		return 0;
	}

	int jitDebug2(void *a, void *b)
	{
		fprintf(stderr, "debug(%p, %p)\n", a, b);
		return 0;
	}

	int jitDebug3(void *a, void *b, void *c)
	{
		fprintf(stderr, "debug(%p, %p, %p)\n", a, b, c);
		return 0;
	}

	int jitDebugCpu(gensim::Processor *cpu)
	{
		fprintf(stderr, "no chain to %x\n", cpu->read_pc());
		return 0;
	}

	void jitTrap()
	{
		asm("int3");
	}

	void jitTrapIf(bool should_trap)
	{
		if (should_trap)
			asm volatile("int3\n");
	}

	void jitAssert(bool cond)
	{
		assert(cond);
	}

	void tmFlushTlb(gensim::Processor *cpu)
	{
		cpu->flush_tlb();
	}

	void tmFlushITlb(gensim::Processor *cpu)
	{
		cpu->flush_itlb();
	}

	void tmFlushDTlb(gensim::Processor *cpu)
	{
		cpu->flush_dtlb();
	}

	void tmFlushITlbEntry(gensim::Processor *cpu, uint32_t addr)
	{
		cpu->flush_itlb_entry(addr);
	}

	void tmFlushDTlbEntry(gensim::Processor *cpu, uint32_t addr)
	{
		cpu->flush_dtlb_entry(addr);
	}

	uint8_t devProbeDevice(gensim::Processor *cpu, uint32_t device_id)
	{
		return (uint8_t)((gensim::Processor*)cpu)->probe_device(device_id);
	}

	uint8_t devWriteDevice(gensim::Processor *cpu, uint32_t device_id, uint32_t addr, uint32_t data)
	{
		return (uint8_t)((gensim::Processor*)cpu)->write_device(device_id, addr, data);
	}

	uint8_t devReadDevice(gensim::Processor *cpu, uint32_t device_id, uint32_t addr, uint32_t* data)
	{
		return (uint8_t)((gensim::Processor*)cpu)->read_device(device_id, addr, *data);
	}

	void sysVerify(gensim::Processor *cpu)
	{
		cpu->GetEmulationModel().GetSystem().CheckVerify();
	}

	void sysPublishInstruction(gensim::Processor *cpu, uint32_t pc, uint32_t type)
	{
		struct {
			uint32_t pc, type;
		} data;
		data.pc = pc;
		data.type = type;

		cpu->GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::InstructionExecute, &data);
	}

	void sysPublishBlock(archsim::util::PubSubContext *pubsub, uint32_t pc, uint32_t count)
	{
		struct {
			uint32_t pc, count;
		} data;
		data.pc = pc;
		data.count = count;

		pubsub->Publish(PubSubType::BlockExecute, &data);
	}

	uint32_t cpuTakeException(gensim::Processor *cpu, uint32_t category, uint32_t data)
	{
		LC_DEBUG2(LogJitFuns) << "CPU Take Exception";
		return (uint32_t)(((gensim::Processor*)cpu)->take_exception(category, data));
	}

	void cpuPushInterruptState(gensim::Processor *cpu, uint32_t state)
	{
		((gensim::Processor*)cpu)->push_interrupt(state);
	}

	void cpuPopInterruptState(gensim::Processor *cpu)
	{
		((gensim::Processor*)cpu)->pop_interrupt();
	}

	void cpuHalt(gensim::Processor *cpu)
	{
		((gensim::Processor*)cpu)->Halt();
	}

	uint32_t cpuHandlePendingAction(gensim::Processor *cpu)
	{
		return ((gensim::Processor*)cpu)->handle_pending_action();
	}

	void cpuReturnToSafepoint(gensim::Processor *cpu)
	{
		cpu->return_to_safepoint();
	}

	void cpuPendInterrupt(gensim::Processor *cpu)
	{
		cpu->pend_interrupt();
	}

	uint32_t cpuTranslate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t *phys_addr)
	{
		return ((archsim::abi::devices::MMU *)cpu->peripherals.GetDeviceByName("mmu"))->Translate(cpu, virt_addr, *phys_addr, MMUACCESSINFO(cpu->in_kernel_mode(), 0, 0));
	}

	uint32_t cpuRead8(gensim::Processor *cpu, uint32_t address, uint8_t& data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuRead8";
		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read8(address, data);
		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead8(cpu, address, data);

		if(rval) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return rval;
	}

	uint32_t cpuRead16(gensim::Processor *cpu, uint32_t address, uint16_t& data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuRead8";
		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read16(address, data);
		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead16(cpu, address, data);

		if(rval) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return rval;
	}

	uint32_t cpuRead32(gensim::Processor *cpu, uint32_t address, uint32_t& data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuRead8";
		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read32(address, data);
		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead32(cpu, address, data);

		if(rval) {
			// XXX ARM HAX
			cpu->take_exception(7, cpu->read_pc()+8);
		}

		return rval;
	}

	uint32_t cpuWrite8(gensim::Processor *cpu, uint32_t address, uint8_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite8";

		int rval = ((gensim::Processor*)cpu)->GetMemoryModel().Write8(address, data);
		if(rval) {
			if(rval < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
				cpuReturnToSafepoint(cpu);
			}
		} else {
			if(cpu->IsTracingEnabled())cpu->GetTraceManager()->Trace_Mem_Write(true, address, data, 1);
		}

		return rval;
	}

	uint32_t cpuWrite16(gensim::Processor *cpu, uint32_t address, uint16_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite16";
		int rval = ((gensim::Processor*)cpu)->GetMemoryModel().Write16(address, data);
		if(rval) {
			if(rval < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
				cpuReturnToSafepoint(cpu);
			}
		} else {
			if(cpu->IsTracingEnabled())cpu->GetTraceManager()->Trace_Mem_Write(true, address, data, 2);
		}
		return rval;
	}

	uint32_t cpuWrite32(gensim::Processor *cpu, uint32_t address, uint32_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite32";
		int rval = ((gensim::Processor*)cpu)->GetMemoryModel().Write32(address, data);
		if(rval) {
			if(rval < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
				cpuReturnToSafepoint(cpu);
			}
		} else {
			if(cpu->IsTracingEnabled())cpu->GetTraceManager()->Trace_Mem_Write(true, address, data);
		}
		return rval;
	}

	uint32_t cpuRead8User(gensim::Processor *cpu, uint32_t address, uint8_t&data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuRead8User";
		uint8_t dl;
		uint32_t rc = cpu->mem_read_8_user(address, dl);
		data = dl;
		if(rc) {
			cpu->take_exception(7, cpu->read_pc()+8);
		}
		return rc;
	}

	uint32_t cpuRead32User(gensim::Processor *cpu, uint32_t address, uint32_t&data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuRead32User";
		int rc = cpu->mem_read_32_user(address, data);
		if(rc) {
			cpu->take_exception(7, cpu->read_pc()+8);
		}
		return rc;
	}

	uint32_t cpuWrite8User(gensim::Processor *cpu, uint32_t address, uint8_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite8User";
		int rc = ((gensim::Processor*)cpu)->GetMemoryModel().Write8User(address, data);
		if(rc) {
			if(rc < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
				cpuReturnToSafepoint(cpu);
			}
		} else {
			if(cpu->IsTracingEnabled())cpu->GetTraceManager()->Trace_Mem_Write(true, address, data, 1);
		}
		return rc;
	}

	uint32_t cpuWrite32User(gensim::Processor *cpu, uint32_t address, uint32_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite32User";
		int rc = cpu->mem_write_32_user(address, data);
		if(rc) {
			if(rc < 1024) {
				// XXX ARM HAX
				cpu->take_exception(7, cpu->read_pc()+8);
			} else {
				cpuReturnToSafepoint(cpu);
			}
		}
		return rc;
	}

	void cpuEnterKernelMode(gensim::Processor *cpu)
	{
		cpu->enter_kernel_mode();
	}

	void cpuEnterUserMode(gensim::Processor *cpu)
	{
		cpu->enter_user_mode();
	}

	void cpuInstructionTick(gensim::Processor *cpu)
	{
		cpu->GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::InstructionExecute, NULL);
	}

	/*
	 * Tracing Infrastructure
	*/

	void cpuTraceString(gensim::Processor *cpu, const char* str, uint32_t do_emit)
	{
		assert(false && "string tracing not currently supported");
	}

	void cpuTraceInstruction(gensim::Processor *cpu, uint32_t pc, uint32_t ir, uint8_t isa_mode, uint8_t irq_mode, uint8_t exec)
	{
		cpu->GetTraceManager()->Trace_Insn(pc, ir, 1, isa_mode, irq_mode, exec);
	}

	void cpuTraceInsnEnd(gensim::Processor *cpu)
	{
		cpu->GetTraceManager()->Trace_End_Insn();
	}

	void cpuTraceRegWrite(gensim::Processor *cpu, uint8_t reg, uint64_t value)
	{
		auto &descriptor = cpu->GetRegisterDescriptor(reg);
		switch(descriptor.RegisterWidth) {
			case 1:
			case 2:
			case 4:
				cpu->GetTraceManager()->Trace_Reg_Write(true, reg, (uint32_t)value);
				break;
			case 8:
				cpu->GetTraceManager()->Trace_Reg_Write(true, reg, (uint64_t)value);
				break;
			default:
				assert(false);
		}

	}

	void cpuTraceRegRead(gensim::Processor *cpu, uint8_t reg, uint64_t value)
	{
		auto &descriptor = cpu->GetRegisterDescriptor(reg);
		switch(descriptor.RegisterWidth) {
			case 1:
			case 2:
			case 4:
				cpu->GetTraceManager()->Trace_Reg_Read(true, reg, (uint32_t)value);
				break;
			case 8:
				cpu->GetTraceManager()->Trace_Reg_Read(true, reg, (uint64_t)value);
				break;
			default:
				assert(false);
		}
	}

	void cpuTraceRegBankWrite(gensim::Processor *cpu, uint8_t bank, uint32_t reg, uint64_t value)
	{
		auto &descriptor = cpu->GetRegisterBankDescriptor(bank);
		switch(descriptor.SizeOfElementInBytes()) {
			case 1:
			case 2:
			case 4:
				cpu->GetTraceManager()->Trace_Bank_Reg_Write(true, bank, reg, (uint32_t)value);
				break;
			case 8:
				cpu->GetTraceManager()->Trace_Bank_Reg_Write(true, bank, reg, (uint64_t)value);
				break;
			default:
				assert(false);
		}
	}

	void cpuTraceRegBankRead(gensim::Processor *cpu, uint8_t bank, uint32_t reg, uint64_t value)
	{
		auto &descriptor = cpu->GetRegisterBankDescriptor(bank);
		switch(descriptor.SizeOfElementInBytes()) {
			case 1:
			case 2:
			case 4:
				cpu->GetTraceManager()->Trace_Bank_Reg_Read(true, bank, reg, (uint32_t)value);
				break;
			case 8:
				cpu->GetTraceManager()->Trace_Bank_Reg_Read(true, bank, reg, (uint64_t)value);
				break;
			default:
				assert(false);
		}
	}

	void cpuTraceOnlyMemRead8(gensim::Processor *cpu, uint32_t addr, uint32_t value)
	{
		cpu->GetTraceManager()->Trace_Mem_Read(true, addr, value & 0xff, 1);
	}

	void cpuTraceOnlyMemRead8s(gensim::Processor *cpu, uint32_t addr, uint32_t value)
	{
		cpu->GetTraceManager()->Trace_Mem_Read(true, addr, value & 0xff, 1);
	}

	void cpuTraceOnlyMemRead16(gensim::Processor *cpu, uint32_t addr, uint32_t value)
	{
		cpu->GetTraceManager()->Trace_Mem_Read(true, addr, value & 0xffff, 2);
	}

	void cpuTraceOnlyMemRead16s(gensim::Processor *cpu, uint32_t addr, uint32_t value)
	{
		cpu->GetTraceManager()->Trace_Mem_Read(true, addr, value & 0xffff, 2);
	}

	void cpuTraceOnlyMemRead32(gensim::Processor *cpu, uint32_t addr, uint32_t value)
	{
		cpu->GetTraceManager()->Trace_Mem_Read(true, addr, value, 4);
	}

	void cpuTraceOnlyMemWrite8(gensim::Processor *cpu, uint32_t addr, uint32_t data)
	{
		cpu->GetTraceManager()->Trace_Mem_Write(true, addr, data & 0xff, 1);
	}

	void cpuTraceOnlyMemWrite16(gensim::Processor *cpu, uint32_t addr, uint32_t data)
	{
		cpu->GetTraceManager()->Trace_Mem_Write(true, addr, data & 0xffff, 2);
	}

	void cpuTraceOnlyMemWrite32(gensim::Processor *cpu, uint32_t addr, uint32_t data)
	{
		cpu->GetTraceManager()->Trace_Mem_Write(true, addr, data, 4);
	}
}
