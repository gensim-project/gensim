#ifndef _jit_funs_h_
#define _jit_funs_h_

namespace gensim
{
	class Processor;
}

struct cpuContext;

extern "C" {

	void cpuSetFeature(gensim::Processor *cpu, uint32_t feature, uint32_t level);
	uint32_t cpuGetRoundingMode(gensim::Processor *cpu);
	void cpuSetRoundingMode(gensim::Processor *cpu, uint32_t mode);
	uint32_t cpuGetFlushMode(gensim::Processor *cpu);
	void cpuSetFlushMode(gensim::Processor *cpu, uint32_t mode);

	uint32_t genc_adc_flags(uint32_t lhs, uint32_t rhs, uint32_t carry_in);

	int jitDebug0();

	int jitDebug1(void *a);

	int jitDebug2(void *a, void *b);

	int jitDebug3(void *a, void *b, void *c);

	uint32_t jitChecksumPage(gensim::Processor *cpu, uint32_t page_base);
	void jitCheckChecksum(uint32_t expected, uint32_t actual);

	uint8_t devProbeDevice(gensim::Processor *cpu, uint32_t device_id);

	uint8_t devWriteDevice(gensim::Processor *cpu, uint32_t device_id, uint32_t addr, uint32_t data);

	uint8_t devReadDevice(gensim::Processor *cpu, uint32_t device_id, uint32_t addr, uint32_t* data);

	void sysVerify(gensim::Processor *cpu);

	uint32_t cpuTakeException(gensim::Processor *cpu, uint32_t category, uint32_t data);

	void cpuPushInterruptState(gensim::Processor *cpu, uint32_t state);

	void cpuPopInterruptState(gensim::Processor *cpu);

	uint32_t cpuHandlePendingAction(gensim::Processor *cpu);

	void cpuReturnToSafepoint(gensim::Processor *cpu);
	void cpuPendInterrupt(gensim::Processor *cpu);

	uint32_t cpuTranslate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t *phys_addr);

	void blkProfile(gensim::Processor *cpu, void *region, uint32_t address);

	// Different versions of memory functions for block jit (implicit return to safepoint on fault) and region jit (no implicit safepoint stuff)
	uint8_t blkRead8(gensim::Processor **cpu, uint32_t address);
	uint16_t blkRead16(gensim::Processor **cpu, uint32_t address);
	uint32_t blkRead32(gensim::Processor **cpu, uint32_t address);

	void blkWrite8(gensim::Processor **cpu, uint32_t address, uint32_t data);
	void blkWrite16(gensim::Processor **cpu, uint32_t address, uint32_t data);
	void blkWrite32(gensim::Processor **cpu, uint32_t address, uint32_t data);

	uint8_t blkRead8User(gensim::Processor **cpu, uint32_t address);
	uint32_t blkRead32User(gensim::Processor **cpu, uint32_t address);
	uint32_t blkWrite8User(gensim::Processor *cpu, uint32_t address, uint8_t data);
	uint32_t blkWrite32User(gensim::Processor *cpu, uint32_t address, uint32_t data);

	uint32_t cpuRead8(gensim::Processor *cpu, uint32_t address, uint8_t& data);
	uint32_t cpuRead16(gensim::Processor *cpu, uint32_t address, uint16_t& data);
	uint32_t cpuRead32(gensim::Processor *cpu, uint32_t address, uint32_t& data);

	uint32_t cpuWrite8(gensim::Processor *cpu, uint32_t address, uint8_t data);
	uint32_t cpuWrite16(gensim::Processor *cpu, uint32_t address, uint16_t data);
	uint32_t cpuWrite32(gensim::Processor *cpu, uint32_t address, uint32_t data);

	uint32_t cpuRead8User(gensim::Processor *cpu, uint32_t address, uint8_t& data);
	uint32_t cpuRead32User(gensim::Processor *cpu, uint32_t address, uint32_t& data);
	uint32_t cpuWrite8User(gensim::Processor *cpu, uint32_t address, uint8_t data);
	uint32_t cpuWrite32User(gensim::Processor *cpu, uint32_t address, uint32_t data);

	void cpuEnterKernelMode(gensim::Processor *cpu);
	void cpuEnterUserMode(gensim::Processor *cpu);
	void cpuInstructionTick(gensim::Processor *cpu);

	void cpuTraceString(gensim::Processor *cpu, const char* str, uint32_t do_emit);

	void cpuTraceInstruction(gensim::Processor *cpu, uint32_t pc, uint32_t ir, uint8_t isa_mode, uint8_t irq_mode, uint8_t exec);

	void cpuTraceInsnEnd(gensim::Processor *cpu);

	void cpuTraceRegWrite(gensim::Processor *cpu, uint8_t reg, uint64_t value);

	void cpuTraceRegRead(gensim::Processor *cpu, uint8_t reg, uint64_t value);

	void cpuTraceRegBankWrite(gensim::Processor *cpu, uint8_t bank, uint32_t reg, uint64_t value);
	void cpuTraceRegBank0Write(gensim::Processor *cpu, uint32_t reg, uint64_t value);

	void cpuTraceRegBankRead(gensim::Processor *cpu, uint8_t bank, uint32_t reg, uint64_t value);
	void cpuTraceRegBank0Read(gensim::Processor *cpu, uint32_t reg, uint64_t value);

	char cpuTraceMemRead8(gensim::Processor *cpu, uint32_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead8(gensim::Processor *cpu, uint32_t addr, uint32_t value);

	char cpuTraceMemRead8s(gensim::Processor *cpu, uint32_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead8s(gensim::Processor *cpu, uint32_t addr, uint32_t value);

	char cpuTraceMemRead16(gensim::Processor *cpu, uint32_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead16(gensim::Processor *cpu, uint32_t addr, uint32_t value);

	char cpuTraceMemRead16s(gensim::Processor *cpu, uint32_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead16s(gensim::Processor *cpu, uint32_t addr, uint32_t value);

	char cpuTraceMemRead32(gensim::Processor *cpu, uint32_t addr, uint32_t* value);

	void cpuTraceOnlyMemRead32(gensim::Processor *cpu, uint32_t addr, uint32_t value);

	char cpuTraceMemWrite8(gensim::Processor *cpu, uint32_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite8(gensim::Processor *cpu, uint32_t addr, uint32_t data);

	char cpuTraceMemWrite16(gensim::Processor *cpu, uint32_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite16(gensim::Processor *cpu, uint32_t addr, uint32_t data);

	char cpuTraceMemWrite32(gensim::Processor *cpu, uint32_t addr, uint32_t data);

	void cpuTraceOnlyMemWrite32(gensim::Processor *cpu, uint32_t addr, uint32_t data);

	void tmFlushITlb(gensim::Processor *cpu);
	void tmFlushDTlb(gensim::Processor *cpu);

	void tmFlushITlbEntry(gensim::Processor *cpu, uint32_t addr);
	void tmFlushDTlbEntry(gensim::Processor *cpu, uint32_t addr);
} // extern "C"

#endif /* _jit_funs_h_ */
