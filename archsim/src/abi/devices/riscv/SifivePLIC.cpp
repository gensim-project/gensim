/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/risc-v/RiscVSystemCoprocessor.h"
#include "abi/devices/riscv/SifivePLIC.h"
#include "core/thread/ThreadInstance.h"
#include "util/LogContext.h"

UseLogContext(LogDevice);
DeclareChildLogContext(LogPLIC, LogDevice, "PLIC");

using namespace archsim::abi::devices::riscv;

using namespace archsim::abi::devices;
static ComponentDescriptor SifivePLICDescriptor ("SifivePLIC", {{"Hart0", ComponentParameter_Thread}, {"Hart1", ComponentParameter_Thread}, {"HartConfig", ComponentParameter_String}});
SifivePLIC::SifivePLIC(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x4000000), IRQController(54), Component(SifivePLICDescriptor)
{
	interrupt_priorities_.resize(GetNumLines());
	interrupt_pending_.resize((GetNumLines() + 31) / 32);
	interrupt_claimed_.resize((GetNumLines() + 31) / 32);
}

SifivePLIC::~SifivePLIC()
{

}

bool SifivePLIC::Initialise()
{
	SetupContexts(GetHartConfig());
	return true;
}

void SifivePLIC::SetupContexts(const std::string& config)
{
	hart_config_.clear();

	uint32_t id = 0;
	for(char c : config) {
		switch(c) {
			case 'M':
			case 'S':
			case 'U':
				hart_config_.push_back(hart_context(id, c));
				break;
			case ',':
				id++;
				break;
		}
	}
}


#define PLIC_GLOBAL_PRIORITY_OFFSET 0x0000
#define PLIC_PENDING_OFFSET 0x1000
#define PLIC_ENABLE_OFFSET 0x2000
#define PLIC_ENABLE_MODE_STRIDE 0x80
#define PLIC_PRIORITY_THRESHOLD_OFFSET 0x200000
#define PLIC_PRIORITY_THRESHOLD_STRIDE 0x1000

struct mode_info {
	uint32_t index;
	uint32_t hart;
	char mode;
};

static mode_info GetModeInfo(const std::string &str, uint32_t index)
{
	uint32_t hart = 0;
	char mode = 0;
	uint32_t skips = index;
	for(int i = 0; i <= skips; ++i) {
		char c = str.at(i);
		switch(c) {
			case ',':
				hart++;
				break;
			case 'M':
			case 'S':
				mode = c;
				break;
			default:
				UNIMPLEMENTED;
		}
	}

	return {index, hart, mode};
}


bool SifivePLIC::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
	std::lock_guard<std::mutex> guard(lock_);
//	ASSERT(size == 8 && (offset & 7 == 0));
//	ASSERT(size == 4);

	if(offset >= PLIC_GLOBAL_PRIORITY_OFFSET && offset < PLIC_PENDING_OFFSET) {
		ASSERT(size == 4);
		ReadPriority(offset, data);
	} else if(offset >= PLIC_PENDING_OFFSET && offset < PLIC_ENABLE_OFFSET) {
		ReadPending(offset, data);
	} else if(offset >= PLIC_ENABLE_OFFSET && offset < PLIC_PRIORITY_THRESHOLD_OFFSET) {
		ReadEnable(offset, data);
	} else if(offset >= PLIC_PRIORITY_THRESHOLD_OFFSET) {
		ReadThreshold(offset, data);
	} else {
		UNIMPLEMENTED;
	}

	UpdateIRQ();

	return true;
}

bool SifivePLIC::ReadPriority(uint32_t offset, uint64_t& data)
{
	int interrupt_id = (offset & 0xfff) >> 2;
	data = interrupt_priorities_.at(interrupt_id);
	return true;
}

bool SifivePLIC::ReadPending(uint32_t offset, uint64_t& data)
{
	UNIMPLEMENTED;
}

bool SifivePLIC::ReadEnable(uint32_t offset, uint64_t& data)
{
	// which hart are we?
	offset -= PLIC_ENABLE_OFFSET;

	// figure out which hart and mode we're talking about
	auto mode_info = GetModeInfo(GetHartConfig(), offset / PLIC_ENABLE_MODE_STRIDE);

	auto &config = hart_config_.at(mode_info.index);
	int index = (offset & 7) / 4;

	data = config.enable.at(index);

	return true;
}

bool SifivePLIC::ReadThreshold(uint32_t offset, uint64_t& data)
{
	// which hart are we?
	offset -= PLIC_PRIORITY_THRESHOLD_OFFSET;

	// figure out which hart and mode we're talking about
	auto mode_info = GetModeInfo(GetHartConfig(), offset / PLIC_PRIORITY_THRESHOLD_STRIDE);

	auto &config = hart_config_.at(mode_info.index);

	if((offset & 7) == 4) {
		// claim
		data = ClaimInterrupt(config);
	} else {
		// threshold
		data = config.threshold;
	}

	return true;
}


bool SifivePLIC::Write(uint32_t offset, uint8_t size, uint64_t data)
{
	std::lock_guard<std::mutex> guard(lock_);
//	ASSERT(size == 8 && (offset & 7 == 0));
//	ASSERT(size == 4);

	if(offset >= PLIC_GLOBAL_PRIORITY_OFFSET && offset < PLIC_PENDING_OFFSET) {
		ASSERT(size == 4);
		WritePriority(offset, data);
	} else if(offset >= PLIC_PENDING_OFFSET && offset < PLIC_ENABLE_OFFSET) {
		WritePending(offset, data);
	} else if(offset >= PLIC_ENABLE_OFFSET && offset < PLIC_PRIORITY_THRESHOLD_OFFSET) {
		WriteEnable(offset, data);
	} else if(offset >= PLIC_PRIORITY_THRESHOLD_OFFSET) {
		WriteThreshold(offset, data);
	} else {
		UNIMPLEMENTED;
	}

	UpdateIRQ();

	return true;
}

bool SifivePLIC::WritePriority(uint32_t offset, uint64_t data)
{
	int interrupt_id = (offset & 0xfff) >> 2;
	interrupt_priorities_.at(interrupt_id) = data;

	LC_DEBUG1(LogPLIC) << "Setting priority of interrupt " << interrupt_id << " to " << data;

	return true;
}

bool SifivePLIC::WriteEnable(uint32_t offset, uint64_t data)
{
	// which hart are we?
	offset -= PLIC_ENABLE_OFFSET;

	// figure out which hart and mode we're talking about
	auto mode_info = GetModeInfo(GetHartConfig(), offset / PLIC_ENABLE_MODE_STRIDE);

	auto &config = hart_config_.at(mode_info.index);
	int index = (offset & 7) / 4;

	LC_DEBUG1(LogPLIC) << "Hart " << mode_info.hart <<": Setting enablement of interrupt block " << index << " in mode " << mode_info.mode << " to " << Address(data);

	config.enable.at(index) = data;

	return true;
}

bool SifivePLIC::WritePending(uint32_t offset, uint64_t data)
{
	UNIMPLEMENTED;
}

bool SifivePLIC::WriteThreshold(uint32_t offset, uint64_t data)
{
	offset -= PLIC_PRIORITY_THRESHOLD_OFFSET;
	int skips = offset / PLIC_PRIORITY_THRESHOLD_STRIDE;
	auto mode_info = GetModeInfo(GetHartConfig(), skips);

	auto &config = hart_config_.at(mode_info.index);

	if((offset & 7) == 0) {
		// write priority threshold
		LC_DEBUG1(LogPLIC) << "Hart " << mode_info.hart <<": Setting threshold of " << mode_info.mode << " to " << data;
		config.threshold = data;
	} else if((offset & 7) == 4) {
		// write claim/complete
		LC_DEBUG1(LogPLIC) << "Hart " << mode_info.hart << ": IRQ Completed (apparently  " << data << ")";
		config.busy = 0;
		CompleteInterrupt(data);
	}

	return true;
}


archsim::core::thread::ThreadInstance* SifivePLIC::GetHart(int i)
{
	switch(i) {
		case 0:
			return GetHart0();
		case 1:
			return GetHart1();
		default:
			UNEXPECTED;
	}
}

bool SifivePLIC::AssertLine(uint32_t line)
{
	uint32_t block = line / 32;
	uint32_t index = line % 32;

	LC_DEBUG1(LogPLIC) << "Global IRQ " << line << " asserted: asserting block "<< block << ", index " << index << " (mask " << std::hex << (1 << index) << ")";

	interrupt_pending_.at(block) |= (1 << index);
	UpdateIRQ();

	return true;
}

bool SifivePLIC::RescindLine(uint32_t line)
{
	uint32_t block = line / 32;
	uint32_t index = line % 32;

	LC_DEBUG1(LogPLIC) << "Global IRQ " << line << " rescinded: rescinding block "<< block << ", index " << index << " (mask " << std::hex << (1 << index) << ")";

	interrupt_pending_.at(block) &= ~(1 << index);
	UpdateIRQ();

	return true;
}

void SifivePLIC::UpdateIRQ()
{
	// figure out, for each hart, which EIP bits should be set
	std::vector<uint32_t> hart_eips(GetHartCount());

	for(auto &context : hart_config_) {

		uint32_t eip = 0;

		for(int word = 0; word < interrupt_pending_.size(); ++word) {
			auto pending = interrupt_pending_.at(word);
			auto claimed = interrupt_claimed_.at(word);

			auto hart_pending = pending & (~claimed) & context.enable.at(word);
			if(hart_pending) {
				LC_DEBUG2(LogPLIC) << "Hart " << context.hart << ":" << context.context << " has interrupts pending in block " << word << ": " << Address(hart_pending);

				switch(context.context) {
					case 'U':
						eip |= (1 << 8);
						break;
					case 'S':
						eip |= (1 << 9);
						break;
					case 'M':
						eip |= (1 << 11);
						break;
				}
			}
		}

		hart_eips.at(context.hart) |= eip;
	}

	for(int hart_id = 0; hart_id < GetHartCount(); ++hart_id) {
		auto hart = GetHart(hart_id);
		auto coproc = (archsim::arch::riscv::RiscVSystemCoprocessor*)hart->GetPeripherals().GetDevice(0);
		uint32_t hart_eip = coproc->ReadMIP() & 0xf00;
		uint32_t pending_eip = hart_eips.at(hart_id);

		// unpend asserted but unpending interrupts
		uint32_t unpend_irqs = hart_eip & ~pending_eip;
		if(unpend_irqs) {
			LC_DEBUG1(LogPLIC) << "Unpending IRQ EIP mask " << Address(unpend_irqs);
			coproc->MachineUnpendInterrupt(unpend_irqs);
		}

		uint32_t pend_irqs = ~hart_eip & pending_eip;
		if(pend_irqs) {
			LC_DEBUG1(LogPLIC) << "Pending IRQ EIP mask " << Address(pend_irqs);
			coproc->MachinePendInterrupt(pend_irqs);
		}
	}
}

void SifivePLIC::RaiseEIPOnHart(uint32_t hart, char context)
{
	archsim::arch::riscv::RiscVSystemCoprocessor* coproc = (archsim::arch::riscv::RiscVSystemCoprocessor*)GetHart(hart)->GetPeripherals().GetDevice(0);

	uint32_t irq_mask = 0;
	switch(context) {
		case 'U':
			irq_mask = 1 << 8;
			break;
		case 'S':
			irq_mask = 1 << 9;
			break;
		case 'M':
			irq_mask = 1 << 11;
			break;
	}

	LC_DEBUG1(LogPLIC) << "Raised IRQ mask " << Address(irq_mask) << " on hart " << hart;

	coproc->MachinePendInterrupt(irq_mask);
}

uint32_t bit_set_forward(uint32_t data)
{
	for(int i = 0; i < 32; ++i) {
		if(data & (1 << i)) {
			return i;
		}
	}
	UNEXPECTED;
}

uint32_t SifivePLIC::ClaimInterrupt(hart_context& context)
{
	uint32_t pending_interrupt = 0;

	// find the highest priority pending interrupt
	for(int word = 0; word < interrupt_pending_.size(); ++word) {
		auto pending = interrupt_pending_.at(word);
		auto claimed = interrupt_claimed_.at(word);

		// check each hart context
		auto hart_pending = pending & (~claimed) & context.enable.at(word);
		if(hart_pending) {
			// ping the hart
			// todo: priority thresholding
			pending_interrupt = (word * 32) + bit_set_forward(hart_pending);
		}
	}

	if(pending_interrupt) {
		uint32_t block = pending_interrupt / 32;
		uint32_t index = pending_interrupt % 32;

		interrupt_claimed_.at(block) |= (1 << index);
		LC_DEBUG1(LogPLIC) << "Hart " << context.hart << ":" << context.context << " claimed IRQ " << pending_interrupt;
	} else {
		LC_DEBUG1(LogPLIC) << "Tried to claim an interrupt, but none was pending!";
	}

	return pending_interrupt;
}

void SifivePLIC::CompleteInterrupt(uint32_t irq)
{
	uint32_t block = irq / 32;
	uint32_t index = irq % 32;

	interrupt_claimed_.at(block) &= ~(1 << index);
}


void SifivePLIC::ClearPendingInterrupt(uint32_t line)
{
	uint32_t block = line / 32;
	uint32_t index = line % 32;

	interrupt_pending_.at(block) &= ~(1 << index);
}
