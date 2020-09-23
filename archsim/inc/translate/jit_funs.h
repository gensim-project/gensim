/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _jit_funs_h_
#define _jit_funs_h_

namespace archsim
{
	namespace core
	{
		namespace thread
		{
			class ThreadInstance;
		}
	}
}

struct cpuContext;

extern "C" {

	void cpuSetFeature(archsim::core::thread::ThreadInstance *cpu, uint32_t feature, uint32_t level);
	uint32_t cpuGetRoundingMode(archsim::core::thread::ThreadInstance *cpu);
	void cpuSetRoundingMode(archsim::core::thread::ThreadInstance *cpu, uint32_t mode);
	uint32_t cpuGetFlushMode(archsim::core::thread::ThreadInstance *cpu);
	void cpuSetFlushMode(archsim::core::thread::ThreadInstance *cpu, uint32_t mode);

	int jitDebug0();

	int jitDebug1(void *a);

	int jitDebug2(void *a, void *b);

	int jitDebug3(void *a, void *b, void *c);

	uint32_t jitChecksumPage(gensim::Processor *cpu, uint32_t page_base);
	void jitCheckChecksum(uint32_t expected, uint32_t actual);

	uint8_t devProbeDevice(gensim::Processor *cpu, uint32_t device_id);

	uint8_t devWriteDevice(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint32_t data);

	uint8_t devReadDevice(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint32_t* data);

	uint8_t devWriteDevice64(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint64_t data);

	uint8_t devReadDevice64(archsim::core::thread::ThreadInstance *cpu, uint32_t device_id, uint32_t addr, uint64_t* data);

	void sysVerify(gensim::Processor *cpu);

	uint32_t cpuTakeException(archsim::core::thread::ThreadInstance *cpu, uint64_t category, uint64_t data);

	void cpuPushInterruptState(gensim::Processor *cpu, uint32_t state);

	void cpuPopInterruptState(gensim::Processor *cpu);

	uint32_t cpuHandlePendingAction(gensim::Processor *cpu);

	void cpuReturnToSafepoint(gensim::Processor *cpu);
	void cpuPendInterrupt(archsim::core::thread::ThreadInstance *cpu);

	uint32_t cpuTranslate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t *phys_addr);
	void cpuTrap(archsim::core::thread::ThreadInstance *cpu);

	void blkProfile(gensim::Processor *cpu, void *region, uint32_t address);

	// Different versions of memory functions for block jit (implicit return to safepoint on fault) and region jit (no implicit safepoint stuff)
	uint8_t blkRead8(archsim::core::thread::ThreadInstance **thread_p, uint64_t address, uint32_t interface);
	uint16_t blkRead16(archsim::core::thread::ThreadInstance **thread_p, uint64_t address, uint32_t interface);
	uint32_t blkRead32(archsim::core::thread::ThreadInstance **thread_p, uint64_t address, uint32_t interface);
	uint64_t blkRead64(archsim::core::thread::ThreadInstance **thread_p, uint64_t address, uint32_t interface);

	void blkWrite8(archsim::core::thread::ThreadInstance **thread_p, uint32_t interface, uint32_t address, uint32_t data);
	void blkWrite16(archsim::core::thread::ThreadInstance **thread_p, uint32_t interface, uint32_t address, uint32_t data);
	void blkWrite32(archsim::core::thread::ThreadInstance **thread_p, uint32_t interface, uint32_t address, uint32_t data);

	uint32_t cpuRead8(gensim::Processor *cpu, uint64_t address, uint8_t& data);
	uint32_t cpuRead16(gensim::Processor *cpu, uint64_t address, uint16_t& data);
	uint32_t cpuRead32(gensim::Processor *cpu, uint64_t address, uint32_t& data);

	uint32_t cpuWrite8(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint8_t data);
	uint32_t cpuWrite16(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint16_t data);
	uint32_t cpuWrite32(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint32_t data);
	uint32_t cpuWrite64(archsim::core::thread::ThreadInstance *cpu, uint32_t interface_id, uint64_t address, uint64_t data);

	void cpuEnterKernelMode(gensim::Processor *cpu);
	void cpuEnterUserMode(gensim::Processor *cpu);
	void cpuInstructionTick(archsim::core::thread::ThreadInstance *cpu);

	void cpuTraceString(gensim::Processor *cpu, const char* str, uint32_t do_emit);

	void cpuTraceInstruction(archsim::core::thread::ThreadInstance *cpu, uint64_t pc, uint32_t ir, uint8_t isa_mode, uint8_t irq_mode, uint8_t exec);

	void cpuTraceInsnEnd(archsim::core::thread::ThreadInstance *cpu);

	void cpuTraceRegWrite(archsim::core::thread::ThreadInstance *cpu, uint8_t reg, uint64_t value);

	void cpuTraceRegRead(archsim::core::thread::ThreadInstance *cpu, uint8_t reg, uint64_t value);

	void cpuTraceRegBankWrite(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint8_t *value_ptr);
	void cpuTraceRegBankWriteValue(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint64_t value);
	void cpuTraceRegBank0Write(archsim::core::thread::ThreadInstance *cpu, uint32_t reg, uint64_t value);

	void cpuTraceRegBankRead(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint8_t *value_ptr);
	void cpuTraceRegBankReadValue(archsim::core::thread::ThreadInstance *cpu, uint8_t bank, uint32_t reg, uint32_t size, uint64_t value);
	void cpuTraceRegBank0Read(archsim::core::thread::ThreadInstance *cpu, uint32_t reg, uint64_t value);

	char cpuTraceMemRead8(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead8(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value);

	char cpuTraceMemRead8s(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead8s(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value);

	char cpuTraceMemRead16(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead16(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value);

	char cpuTraceMemRead16s(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead16s(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value);

	char cpuTraceMemRead32(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead32(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t value);

	void cpuTraceOnlyMemRead64(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint64_t value);

	char cpuTraceMemWrite8(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite8(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data);

	char cpuTraceMemWrite16(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite16(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data);

	char cpuTraceMemWrite32(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite32(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite64(archsim::core::thread::ThreadInstance *cpu, uint64_t addr, uint64_t data);

	void tmFlushITlb(gensim::Processor *cpu);
	void tmFlushDTlb(gensim::Processor *cpu);

	void tmFlushITlbEntry(gensim::Processor *cpu, uint64_t addr);
	void tmFlushDTlbEntry(gensim::Processor *cpu, uint64_t addr);

#define RETURNTYPE_OF0(x) decltype(std::declval<archsim::core::thread::ThreadInstance>().fn_##x())
#define RETURNTYPE_OF1(x, p1) decltype(std::declval<archsim::core::thread::ThreadInstance>().fn_##x(std::declval<p1>()))
#define RETURNTYPE_OF2(x, p1, p2) decltype(std::declval<archsim::core::thread::ThreadInstance>().fn_##x(std::declval<p1>(), std::declval<p2>()))

#define JIT_SHUNT0(x) static RETURNTYPE_OF0(x) jit_##x(archsim::core::thread::ThreadInstance *cpu) { return cpu->fn_##x(); }
#define JIT_SHUNT1(x, p1) static  RETURNTYPE_OF1(x, p1) jit_##x(archsim::core::thread::ThreadInstance *cpu, p1 param1) { return cpu->fn_##x(param1); }
#define JIT_SHUNT2(x, p1, p2) static  RETURNTYPE_OF2(x, p1, p2) jit_##x(archsim::core::thread::ThreadInstance *cpu, p1 param1, p2 param2) { return cpu->fn_##x(param1, param2); }

	JIT_SHUNT1(mmu_notify_asid_change, uint32_t);
	JIT_SHUNT1(mmu_flush_va, uint64_t);
	JIT_SHUNT0(mmu_notify_pgt_change);
	JIT_SHUNT0(mmu_flush_all);

	JIT_SHUNT2(__builtin_cmpf32e_flags, float, float);
	JIT_SHUNT2(__builtin_cmpf32_flags, float, float);
	JIT_SHUNT2(__builtin_cmpf64e_flags, double, double);
	JIT_SHUNT2(__builtin_cmpf64_flags, double, double);

	JIT_SHUNT2(__builtin_fcvt_f32_s32, float, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f32_s64, float, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f64_s32, double, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f64_s64, double, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f32_u32, float, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f32_u64, float, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f64_u32, double, uint8_t);
	JIT_SHUNT2(__builtin_fcvt_f64_u64, double, uint8_t);

	JIT_SHUNT2(__builtin_f32_round, float, uint8_t);
	JIT_SHUNT2(__builtin_f64_round, double, uint8_t);

	JIT_SHUNT2(__builtin_polymul8, uint8_t, uint8_t);
	JIT_SHUNT2(__builtin_polymul64, uint64_t, uint64_t);

} // extern "C"

#endif /* _jit_funs_h_ */
