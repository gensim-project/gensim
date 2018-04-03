/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "abi/UserEmulationModel.h"
#include "abi/devices/SerialPort.h"
#include "util/ComponentManager.h"
#include "abi/Address.h"
#include "gensim/gensim_decode_context.h"
#include "gensim/gensim_processor.h"
#include "gensim/gensim_decode.h"
#include "abi/memory/MemoryModel.h"

#include <unordered_map>
#include <set>

using namespace archsim::abi;

class UMMemoryModel : public archsim::abi::memory::ContiguousMemoryModel {
	uint32_t Read32(archsim::abi::memory::guest_addr_t addr, uint32_t& data) override {
		ContiguousMemoryModel::Read32(addr, data);
		
		data = (data & 0xff) << 24 | ((data >> 8) & 0xff) << 16 | ((data >> 16) & 0xff) << 8 | ((data >> 24) & 0xff);
		return 0;
	}
	
	uint32_t Write32(archsim::abi::memory::guest_addr_t addr, uint32_t data) override {
		data = (data & 0xff) << 24 | ((data >> 8) & 0xff) << 16 | ((data >> 16) & 0xff) << 8 | ((data >> 24) & 0xff);
		return ContiguousMemoryModel::Write32(addr, data);
	}
	
	uint32_t Fetch32(archsim::abi::memory::guest_addr_t addr, uint32_t& data) override {
		return Read32(addr, data);
	}

};

class UMDecodeContext : public gensim::DecodeContext {
public:
	UMDecodeContext(gensim::Processor *cpu) : gensim::DecodeContext(cpu) {}
	virtual uint32_t DecodeSync(archsim::Address address, uint32_t mode, gensim::BaseDecode& target) { 
		return GetCPU()->DecodeInstr(address.Get(), mode, target);
	}
};

typedef struct {
	uint32_t start, size;
} free_area_t;

class UMEmulationModel : public archsim::abi::UserEmulationModel {
public:
	UMEmulationModel() : UserEmulationModel("um") {}
	virtual ~UMEmulationModel() {}

	uint32_t Allocate(uint32_t size_bytes) {
		
		if(size_bytes > max_allocation_bytes_block_) {
			size_bytes = max_allocation_bytes_block_;
		}
		
		// Make extra space for the metadata (allocation size)
		size_bytes += 4;

		LC_DEBUG1(LogCPU) << "Getting allocation for size " << size_bytes;
		
		uint32_t allocation;
		bool found = false;
		// Look through the current free areas
		for(auto it = free_areas_.begin(); it != free_areas_.end(); ++it) {
			auto &i = *it;
			
			// Is this free area big enough for our new allocation?
			if(i.size >= size_bytes) {
				
				// Allocate into this free area
				allocation = i.start;
				i.start += size_bytes;
				i.size -= size_bytes;
				
				LC_DEBUG1(LogCPU) << "Found a free area at " << std::hex << i.start;
				
				// The free area might now be empty (0 bytes). If so, delete it
				if(i.size == 0) {
					free_areas_.erase(it);
					LC_DEBUG1(LogCPU) << "The allocation is now empty";
				}
				
				found = true;
				break;
			}	
		}
		
		// If we couldn't find a free area, create a new one on the end of known memory
		if(!found) {
			LC_DEBUG1(LogCPU) << "No free areas found, creating a new allocation at " << std::hex << next_allocation_;
			allocation = next_allocation_;
			next_allocation_ += size_bytes;
		}
		
		// Write the metadata (allocation size) to the start of the allocation
		GetMemoryModel().Write32(allocation, size_bytes);
		allocation += 4;
		
		return allocation;
	}
	
	void Abandon(uint32_t allocation)
	{
		uint32_t size;
		
		// Look for the metadata at the start of the allocation and read it in
		GetMemoryModel().Read32(allocation-4, size);
		
		// Then create a new free area representing this abandonment
		free_areas_.push_back({allocation-4, size});
		
		// Merge free areas together if possible
		MergeFreeAreas();
	}
	
	void MergeFreeAreas() {
		// nothing for now
	}
	
	virtual ExceptionAction HandleException(gensim::Processor& cpu, unsigned int category, unsigned int data) {
		uint8_t A = data & 0xff;
		uint8_t B = (data >> 8) & 0xff;
		uint8_t C = (data >> 16) & 0xff;
		uint32_t *RB = RB_ptr_;
		
		switch(category) {
			case 0: // halt
				LC_ERROR(LogCPU) << "Halt.";
				GetBootCore()->Halt();
				return AbortSimulation;
			case 1: // allocate
			{
				// size is in C
				uint32_t size = RB[C];
				uint32_t size_bytes = size * 4;
				
				RB[B] = Allocate(size_bytes);
				break;
			}	
			case 2: // abandon
			{
				uint32_t allocation = RB[C];
				Abandon(allocation);
				break;
			}
			case 3: // output
				serial_port_->WriteChar(RB[C]);
				break;
			case 4: // input
			{
				char c;
				serial_port_->ReadChar(c);
				RB[C] = c;
				break;
			}
			case 5: // loadprogram
			{
				if(RB[B] != 0) {
					uint32_t size_bytes;
					
					// Load the size of the array that we're switching to
					GetMemoryModel().Read32(RB[B]-4, size_bytes);
					
					host_addr_t src, dest;
					GetMemoryModel().LockRegion(RB[B], size_bytes, src);
					GetMemoryModel().LockRegion(0,     size_bytes, dest);
					
					memcpy(dest, src, size_bytes);
					
					GetSystem().GetProfileManager().Invalidate();
					GetSystem().GetPubSub().Publish(PubSubType::FlushTranslations, (void*)0);
					
					LC_DEBUG1(LogCPU) << "loadprogram caused segment change";
				}
				
				GetBootCore()->write_pc(RB[C] * 4);
				
				break;
			}	
			
			default:
				LC_ERROR(LogCPU) << "Unrecognized syscall " << std::dec << category;
				return AbortSimulation;
		}
		
		return ResumeNext;
	}
	
	virtual gensim::DecodeContext* GetNewDecodeContext(gensim::Processor& cpu) { return new UMDecodeContext(&cpu); }

	bool CreateMemoryModel() override { SetMemoryModel(new UMMemoryModel()); return true; }
	

	virtual bool PrepareBoot(System& system) {
		// load array 0
		FILE *file = fopen(archsim::options::TargetBinary.GetValue().c_str(), "r");
		char c;
		uint32_t addr = 0;
		
		std::vector<uint8_t> bytes;
		while(fread(&c, 1, 1, file) == 1) {
			bytes.push_back(c);
		}
		
		GetMemoryModel().GetMappingManager()->MapRegion(0, max_allocation_bytes_block_, archsim::abi::memory::RegFlagReadWriteExecute, "Program");
		GetMemoryModel().WriteN(0, bytes.data(), bytes.size());
		zero_allocation_size_ = bytes.size();
		
		GetMemoryModel().GetMappingManager()->MapRegion(0x80000000, 0x80000000, archsim::abi::memory::RegFlagReadWrite, "Data");
		
		GetBootCore()->write_pc(0);
		RB_ptr_ = (uint32_t*)GetBootCore()->GetRegisterBankDescriptor("RB").GetBankDataStart();
		next_allocation_ = 0x80000000;
		
		serial_port_ = new archsim::abi::devices::ConsoleSerialPort();
		serial_port_->Open();
		
		return true;
	}

private:
	uint32_t zero_allocation_size_;
	uint32_t *RB_ptr_;
	
	std::vector<free_area_t> free_areas_;
	uint32_t next_allocation_;
	
	const uint32_t max_allocation_bytes_block_ = 128 * 1024 * 1024;
	
	archsim::abi::devices::ConsoleSerialPort *serial_port_;
};


class UMDecodeTranslationContext : public gensim::DecodeTranslateContext
{
	void Translate(gensim::BaseDecode& insn, gensim::DecodeContext& decode, captive::shared::IRBuilder &builder) override
	{
		// nothing necessary here
	}

};

RegisterComponent(archsim::abi::EmulationModel, UMEmulationModel, "um", "UM emulation model");

RegisterComponent(gensim::DecodeContext, UMDecodeContext, "um", "UM emulation model", gensim::Processor*);
RegisterComponent(gensim::DecodeTranslateContext, UMDecodeTranslationContext, "um", "UM emulation model");