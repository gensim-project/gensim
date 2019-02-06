/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/riscv/SifivePLIC.h"
#include "util/LogContext.h"

UseLogContext(LogDevice);

using namespace archsim::abi::devices::riscv;

using namespace archsim::abi::devices;
static ComponentDescriptor SifivePLICDescriptor ("SifivePLIC", {{"Hart0", ComponentParameter_Thread}, {"HartConfig", ComponentParameter_String}});
SifivePLIC::SifivePLIC(EmulationModel& parent, Address base_address) : MemoryComponent(parent, base_address, 0x4000000), IRQController(54), Component(SifivePLICDescriptor)
{
	interrupt_priorities_.resize(GetNumLines());
	hart_config_.resize(1);
}

SifivePLIC::~SifivePLIC()
{

}

bool SifivePLIC::Initialise()
{
	return true;
}

#define PLIC_GLOBAL_PRIORITY_OFFSET 0x0000
#define PLIC_PENDING_OFFSET 0x1000
#define PLIC_ENABLE_OFFSET 0x2000
#define PLIC_ENABLE_MODE_STRIDE 0x80
#define PLIC_PRIORITY_THRESHOLD_OFFSET 0x200000
#define PLIC_PRIORITY_THRESHOLD_STRIDE 0x1000

struct mode_info {
	uint32_t hart;
	char mode;
};

static mode_info GetModeInfo(const std::string &str, int index)
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

	return {hart, mode};
}


bool SifivePLIC::Read(uint32_t offset, uint8_t size, uint64_t& data)
{
//	ASSERT(size == 8 && (offset & 7 == 0));
//	ASSERT(size == 4);

	if(offset >= PLIC_GLOBAL_PRIORITY_OFFSET && offset < PLIC_PENDING_OFFSET) {
		ASSERT(size == 4);
		return ReadPriority(offset, data);
	} else if(offset >= PLIC_PENDING_OFFSET && offset < PLIC_ENABLE_OFFSET) {
		return ReadPending(offset, data);
	} else if(offset >= PLIC_ENABLE_OFFSET && offset < PLIC_PRIORITY_THRESHOLD_OFFSET) {
		return ReadEnable(offset, data);
	} else if(offset >= PLIC_PRIORITY_THRESHOLD_OFFSET) {
		return ReadThreshold(offset, data);
	} else {
		UNIMPLEMENTED;
	}

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

	if(mode_info.hart != 0) {
		UNIMPLEMENTED;
	}

	auto &config = hart_config_.at(mode_info.hart);
	int index = (offset & 7) / 4;

	switch(mode_info.mode) {
		case 'M':
			data = config.enable_m.at(index);
			break;
		case 'S':
			data = config.enable_s.at(index);
			break;
		default:
			UNEXPECTED;
	}

	return true;
}

bool SifivePLIC::ReadThreshold(uint32_t offset, uint64_t& data)
{
	UNIMPLEMENTED;
}


bool SifivePLIC::Write(uint32_t offset, uint8_t size, uint64_t data)
{
//	ASSERT(size == 8 && (offset & 7 == 0));
//	ASSERT(size == 4);

	if(offset >= PLIC_GLOBAL_PRIORITY_OFFSET && offset < PLIC_PENDING_OFFSET) {
		ASSERT(size == 4);
		return WritePriority(offset, data);
	} else if(offset >= PLIC_PENDING_OFFSET && offset < PLIC_ENABLE_OFFSET) {
		return WritePending(offset, data);
	} else if(offset >= PLIC_ENABLE_OFFSET && offset < PLIC_PRIORITY_THRESHOLD_OFFSET) {
		return WriteEnable(offset, data);
	} else if(offset >= PLIC_PRIORITY_THRESHOLD_OFFSET) {
		return WriteThreshold(offset, data);
	} else {
		UNIMPLEMENTED;
	}

	return true;
}

bool SifivePLIC::WritePriority(uint32_t offset, uint64_t data)
{
	int interrupt_id = (offset & 0xfff) >> 2;
	interrupt_priorities_.at(interrupt_id) = data;
	return true;
}

bool SifivePLIC::WriteEnable(uint32_t offset, uint64_t data)
{
	// which hart are we?
	offset -= PLIC_ENABLE_OFFSET;

	// figure out which hart and mode we're talking about
	auto mode_info = GetModeInfo(GetHartConfig(), offset / PLIC_ENABLE_MODE_STRIDE);

	if(mode_info.hart != 0) {
		UNIMPLEMENTED;
	}

	auto &config = hart_config_.at(mode_info.hart);
	int index = (offset & 7) / 4;

	switch(mode_info.mode) {
		case 'M':
			config.enable_m.at(index) = data;
			break;
		case 'S':
			config.enable_s.at(index) = data;
			break;
		default:
			UNEXPECTED;
	}

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

	auto &config = hart_config_.at(mode_info.hart);

	if((offset & 7) == 0) {
		// write priority threshold
		switch(mode_info.mode) {
			case 'M':
				config.threshold_m = data;
				break;
			case 'S':
				config.threshold_s = data;
				break;
			default:
				UNEXPECTED;
		}
	} else if((offset & 7) == 4) {
		// write claim/complete
		UNIMPLEMENTED;
	}

	return true;
}


archsim::core::thread::ThreadInstance* SifivePLIC::GetHart(int i)
{
	ASSERT(i == 0);
	return GetHart0();
}

bool SifivePLIC::AssertLine(uint32_t line)
{
	UNIMPLEMENTED;
	return false;
}

bool SifivePLIC::RescindLine(uint32_t line)
{
	UNIMPLEMENTED;
	return false;
}
