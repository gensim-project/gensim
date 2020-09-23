/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "define.h"

#include "abi/devices/MMU.h"
#include "abi/memory/MemoryModel.h"

#include "core/MemoryInterface.h"
#include "core/thread/ThreadInstance.h"

#include "util/LogContext.h"

#include "translate/jit_funs.h"
#include "system.h"

#include <cfenv>
#include <setjmp.h>

namespace gensim
{
	class Processor;
}

UseLogContext(LogCPU);
DeclareChildLogContext(LogJitFuns, LogCPU, "LogJITFuns");
UseLogContext(LogJitFuns)
extern "C" {
	void cpuSetFeature(archsim::core::thread::ThreadInstance *cpu, uint32_t feature, uint32_t level)
	{
		cpu->GetFeatures().SetFeatureLevel(feature, level);
	}

	uint32_t cpuGetRoundingMode(archsim::core::thread::ThreadInstance *cpu)
	{
		return (uint32_t)cpu->GetFPState().GetRoundingMode();
	}
	void cpuSetRoundingMode(archsim::core::thread::ThreadInstance *cpu, uint32_t mode)
	{
		return cpu->GetFPState().SetRoundingMode((archsim::core::thread::RoundingMode)mode);
	}

	uint32_t cpuGetFlushMode(archsim::core::thread::ThreadInstance *cpu)
	{
		return (uint32_t)cpu->GetFPState().GetFlushMode();
	}
	void cpuSetFlushMode(archsim::core::thread::ThreadInstance *cpu, uint32_t mode)
	{
		return cpu->GetFPState().SetFlushMode((archsim::core::thread::FlushMode)mode);
	}

	uint8_t cpuGetExecMode(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
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
		return 0;
	}

	void jitTrap()
	{
		throw std::logic_error("trap.");
	}

	void jitTrapIf(bool should_trap)
	{
		if (should_trap)
			throw std::logic_error("trap.");
	}

	void jitAssert(bool cond)
	{
		assert(cond);
	}

	void tmFlushTlb(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		cpu->flush_tlb();
	}

	void tmFlushITlb(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		cpu->flush_itlb();
	}

	void tmFlushDTlb(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		cpu->flush_dtlb();
	}

	void tmFlushITlbEntry(gensim::Processor *cpu, uint64_t addr)
	{
		UNIMPLEMENTED;
//		cpu->flush_itlb_entry(addr);
	}

	void tmFlushDTlbEntry(gensim::Processor *cpu, uint64_t addr)
	{
		UNIMPLEMENTED;
//		cpu->flush_dtlb_entry(addr);
	}

	uint8_t devProbeDevice(gensim::Processor *cpu, uint32_t device_id)
	{
		UNIMPLEMENTED;
	}

	uint8_t devWriteDevice(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint32_t data)
	{
		return cpu->GetPeripherals().AttachedPeripherals.at(device_id)->Write32(addr, data);
	}

	uint8_t devReadDevice(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint32_t* data)
	{
		return cpu->GetPeripherals().AttachedPeripherals.at(device_id)->Read32(addr, *data);
	}

	uint8_t devWriteDevice64(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint64_t data)
	{
		return cpu->GetPeripherals().AttachedPeripherals.at(device_id)->Write64(addr, data);
	}

	uint8_t devReadDevice64(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint64_t* data)
	{
		return cpu->GetPeripherals().AttachedPeripherals.at(device_id)->Read64(addr, *data);
	}

	void sysVerify(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
	}

	void sysPublishInstruction(gensim::Processor *cpu, uint32_t pc, uint32_t type)
	{
//		struct {
//			uint32_t pc, type;
//		} data;
//		data.pc = pc;
//		data.type = type;

		UNIMPLEMENTED;
//		cpu->GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::InstructionExecute, &data);
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

	uint32_t cpuTakeException(archsim::core::thread::ThreadInstance *thread, uint64_t category, uint64_t data)
	{
		LC_DEBUG2(LogJitFuns) << "CPU Take Exception";
		auto result = thread->TakeException(category, data);
		return (int)result;
	}

	void cpuPushInterruptState(gensim::Processor *cpu, uint32_t state)
	{
		UNIMPLEMENTED;
//		((gensim::Processor*)cpu)->push_interrupt(state);
	}

	void cpuPopInterruptState(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		((gensim::Processor*)cpu)->pop_interrupt();
	}

	void cpuHalt(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		((gensim::Processor*)cpu)->Halt();
	}

	uint32_t cpuHandlePendingAction(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
	}

	void cpuReturnToSafepoint(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
	}

	void cpuPendInterrupt(archsim::core::thread::ThreadInstance*cpu)
	{
		cpu->PendIRQ();
	}

	uint32_t cpuTranslate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t *phys_addr)
	{
		UNIMPLEMENTED;
//		return ((archsim::abi::devices::MMU *)cpu->peripherals.GetDeviceByName("mmu"))->Translate(cpu, virt_addr, *phys_addr, MMUACCESSINFO(cpu->in_kernel_mode(), 0, 0));
	}

	void cpuTrap(archsim::core::thread::ThreadInstance *thread)
	{
		throw std::logic_error("Trap.");
	}

	uint32_t cpuRead8(gensim::Processor *cpu, uint64_t address, uint8_t& data)
	{
		UNIMPLEMENTED;
//		LC_DEBUG2(LogJitFuns) << "cpuRead8";
//		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read8(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead8(cpu, address, data);
//
//		if(rval) {
//			// XXX ARM HAX
//			cpu->take_exception(7, cpu->read_pc()+8);
//		}
//
//		return rval;
	}

	uint32_t cpuRead16(gensim::Processor *cpu, uint64_t address, uint16_t& data)
	{
		UNIMPLEMENTED;
//		LC_DEBUG2(LogJitFuns) << "cpuRead8";
//		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read16(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead16(cpu, address, data);
//
//		if(rval) {
//			// XXX ARM HAX
//			cpu->take_exception(7, cpu->read_pc()+8);
//		}
//
//		return rval;
	}

	uint32_t cpuRead32(gensim::Processor *cpu, uint64_t address, uint32_t& data)
	{
		UNIMPLEMENTED;

//		LC_DEBUG2(LogJitFuns) << "cpuRead8";
//		auto rval = ((gensim::Processor*)cpu)->GetMemoryModel().Read32(address, data);
//		if(cpu->IsTracingEnabled()) cpuTraceOnlyMemRead32(cpu, address, data);
//
//		if(rval) {
//			// XXX ARM HAX
//			cpu->take_exception(7, cpu->read_pc()+8);
//		}
//
//		return rval;
	}

	uint32_t cpuWrite8(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint8_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite8 " << interface_id << " " << archsim::Address(address) << " " << (uint32_t)data;

		auto &interface = cpu->GetMemoryInterface(interface_id);
		auto rval = interface.Write8(archsim::Address(address), data);
		if(rval != archsim::MemoryResult::OK) {
			cpu->TakeMemoryException(interface, archsim::Address(address));
		} else {
			// OK!
		}
		return 0;
	}

	uint32_t cpuWrite16(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint16_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite16 " << interface_id << " " << archsim::Address(address) << " " << (uint32_t)data;
		auto &interface = cpu->GetMemoryInterface(interface_id);
		auto rval = interface.Write16(archsim::Address(address), data);
		if(rval != archsim::MemoryResult::OK) {
			cpu->TakeMemoryException(interface, archsim::Address(address));
		} else {
			// OK!
		}
		return 0;
	}

	uint32_t cpuWrite32(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint32_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite32 " << interface_id << " " << archsim::Address(address) << " " << (uint32_t)data;
		auto &interface = cpu->GetMemoryInterface(interface_id);
		auto rval = interface.Write32(archsim::Address(address), data);
		if(rval != archsim::MemoryResult::OK) {
			cpu->TakeMemoryException(interface, archsim::Address(address));
		} else {
			// OK!
		}
		return 0;
	}
	uint32_t cpuWrite64(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint64_t data)
	{
		LC_DEBUG2(LogJitFuns) << "cpuWrite64 " << interface_id << " " << archsim::Address(address) << " " << (uint64_t)data;
		auto &interface = cpu->GetMemoryInterface(interface_id);
		auto rval = interface.Write64(archsim::Address(address), data);
		if(rval != archsim::MemoryResult::OK) {
			cpu->TakeMemoryException(interface, archsim::Address(address));
		} else {
			// OK!
		}
		return 0;
	}

	uint32_t cpuRead8User(gensim::Processor *cpu, uint64_t address, uint8_t&data)
	{
		UNIMPLEMENTED;
//		LC_DEBUG2(LogJitFuns) << "cpuRead8User";
//		uint8_t dl;
//		uint32_t rc = cpu->mem_read_8_user(address, dl);
//		data = dl;
//		if(rc) {
//			cpu->take_exception(7, cpu->read_pc()+8);
//		}
//		return rc;
	}

	uint32_t cpuRead32User(gensim::Processor *cpu, uint64_t address, uint32_t&data)
	{
		UNIMPLEMENTED;
//		LC_DEBUG2(LogJitFuns) << "cpuRead32User";
//		int rc = cpu->mem_read_32_user(address, data);
//		if(rc) {
//			cpu->take_exception(7, cpu->read_pc()+8);
//		}
//		return rc;
	}

	uint32_t cpuWrite8User(gensim::Processor *cpu, uint64_t address, uint8_t data)
	{
		UNIMPLEMENTED;
//		LC_DEBUG2(LogJitFuns) << "cpuWrite8User";
//		int rc = ((gensim::Processor*)cpu)->GetMemoryModel().Write8User(address, data);
//		if(rc) {
//			if(rc < 1024) {
//				// XXX ARM HAX
//				cpu->take_exception(7, cpu->read_pc()+8);
//			} else {
//				cpuReturnToSafepoint(cpu);
//			}
//		} else {
//			if(cpu->IsTracingEnabled())cpu->GetTraceManager()->Trace_Mem_Write(true, address, data, 1);
//		}
//		return rc;
	}

	uint32_t cpuWrite32User(gensim::Processor *cpu, uint64_t address, uint32_t data)
	{
		UNIMPLEMENTED;
//		LC_DEBUG2(LogJitFuns) << "cpuWrite32User";
//		int rc = cpu->mem_write_32_user(address, data);
//		if(rc) {
//			if(rc < 1024) {
//				// XXX ARM HAX
//				cpu->take_exception(7, cpu->read_pc()+8);
//			} else {
//				cpuReturnToSafepoint(cpu);
//			}
//		}
//		return rc;
	}

	void cpuEnterKernelMode(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		cpu->enter_kernel_mode();
	}

	void cpuEnterUserMode(gensim::Processor *cpu)
	{
		UNIMPLEMENTED;
//		cpu->enter_user_mode();
	}

	void cpuInstructionTick(archsim::core::thread::ThreadInstance *cpu)
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

	void cpuTraceInstruction(archsim::core::thread::ThreadInstance *cpu, uint64_t pc, uint32_t ir, uint8_t isa_mode, uint8_t irq_mode, uint8_t exec)
	{
		cpu->GetTraceSource()->Trace_Insn(pc, ir, 1, isa_mode, irq_mode, exec);
	}

	void cpuTraceInsnEnd(archsim::core::thread::ThreadInstance *cpu)
	{
		cpu->GetTraceSource()->Trace_End_Insn();
	}

	void cpuTraceRegWrite(archsim::core::thread::ThreadInstance *cpu, uint8_t reg, uint64_t value)
	{
		auto &descriptor = cpu->GetArch().GetRegisterFileDescriptor().GetByID(reg);
		// TODO: unify reg descriptor IDs in gensim and archsim
		switch(8) {
			case 1:
			case 2:
			case 4:
				cpu->GetTraceSource()->Trace_Reg_Write(true, reg, (uint32_t)value);
				break;
			case 8:
				cpu->GetTraceSource()->Trace_Reg_Write(true, reg, (uint64_t)value);
				break;
			default:
				assert(false);
		}

	}

	void cpuTraceRegRead(archsim::core::thread::ThreadInstance *cpu, uint8_t reg, uint64_t value)
	{
		auto &descriptor = cpu->GetArch().GetRegisterFileDescriptor().GetByID(reg);
		// TODO: unify reg descriptor IDs in gensim and archsim
		switch(8) {
			case 1:
			case 2:
			case 4:
				cpu->GetTraceSource()->Trace_Reg_Read(true, reg, (uint32_t)value);
				break;
			case 8:
				cpu->GetTraceSource()->Trace_Reg_Read(true, reg, (uint64_t)value);
				break;
			default:
				assert(false);
		}
	}

	void cpuTraceRegBankWrite(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint8_t *value_ptr)
	{
		cpu->GetTraceSource()->Trace_Bank_Reg_Write(true, bank, reg, (char*)value_ptr, size);
	}

	void cpuTraceRegBankRead(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint8_t *value_ptr)
	{
		cpu->GetTraceSource()->Trace_Bank_Reg_Read(true, bank, reg, (char*)value_ptr, size);
	}

	void cpuTraceRegBankWriteValue(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint64_t value)
	{
		cpu->GetTraceSource()->Trace_Bank_Reg_Write(true, bank, reg, (char*)&value, size);
	}

	void cpuTraceRegBankReadValue(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint64_t value)
	{
		cpu->GetTraceSource()->Trace_Bank_Reg_Read(true, bank, reg, (char*)&value, size);
	}

	void cpuTraceOnlyMemRead8(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value)
	{
		cpu->GetTraceSource()->Trace_Mem_Read(true, addr, value & 0xff, 1);
	}

	void cpuTraceOnlyMemRead16(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value)
	{
		cpu->GetTraceSource()->Trace_Mem_Read(true, addr, value & 0xffff, 2);
	}

	void cpuTraceOnlyMemRead32(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value)
	{
		cpu->GetTraceSource()->Trace_Mem_Read(true, addr, value & 0xffffffff, 4);
	}

	void cpuTraceOnlyMemRead64(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint64_t value)
	{
		cpu->GetTraceSource()->Trace_Mem_Read(true, addr, value, 8);
	}

	void cpuTraceOnlyMemWrite8(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data)
	{
		cpu->GetTraceSource()->Trace_Mem_Write(true, addr, data & 0xff, 1);
	}

	void cpuTraceOnlyMemWrite16(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data)
	{
		cpu->GetTraceSource()->Trace_Mem_Write(true, addr, data & 0xffff, 2);
	}

	void cpuTraceOnlyMemWrite32(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data)
	{
		cpu->GetTraceSource()->Trace_Mem_Write(true, addr, data & 0xffffffff, 4);
	}
	void cpuTraceOnlyMemWrite64(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint64_t data)
	{
		cpu->GetTraceSource()->Trace_Mem_Write(true, addr, data, 8);
	}
}
