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

#include <map>
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


};

class UMDecodeContext : public gensim::DecodeContext {
public:
	UMDecodeContext(gensim::Processor *cpu) : gensim::DecodeContext(cpu) {}
	virtual uint32_t DecodeSync(archsim::Address address, uint32_t mode, gensim::BaseDecode& target) { 
		// convert big to small endian :-)
		uint8_t data[4];
		
		GetCPU()->GetMemoryModel().Read8(address.Get(), data[3]);
		GetCPU()->GetMemoryModel().Read8(address.Get()+1, data[2]);
		GetCPU()->GetMemoryModel().Read8(address.Get()+2, data[1]);
		GetCPU()->GetMemoryModel().Read8(address.Get()+3, data[0]);
		
		GetCPU()->DecodeInstrIr(*(uint32_t*)&data, mode, target); 
		return 0;
	}
};

class UMEmulationModel : public archsim::abi::UserEmulationModel {
public:
	UMEmulationModel() : UserEmulationModel("um") {}
	virtual ~UMEmulationModel() {}

	virtual ExceptionAction HandleException(gensim::Processor& cpu, unsigned int category, unsigned int data) {
		uint8_t A = data & 0xff;
		uint8_t B = (data >> 8) & 0xff;
		uint8_t C = (data >> 16) & 0xff;
		uint32_t *RB = (uint32_t*)GetBootCore()->GetRegisterBankDescriptor(0).GetBankDataStart();
		
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
				
				if(size_bytes > max_allocation_bytes_block_) {
					size_bytes = max_allocation_bytes_block_;
				}
				
				uint32_t pages = archsim::RegionArch::PageCountOf(size_bytes);
				
				uint32_t allocation;
				
				LC_DEBUG4(LogCPU) << "Allocating " << pages << " pages for " << size_bytes;
				
				if(free_allocations_[pages].size()) {
					allocation = free_allocations_[pages].back();
					free_allocations_[pages].pop_back();
					
					LC_DEBUG4(LogCPU) << "Found a previous allocation at " <<std::hex << allocation;
				} else {
					allocation = next_allocation_;
					next_allocation_ += pages * archsim::RegionArch::PageSize;
					LC_DEBUG4(LogCPU) << "Created a new allocation at " << std::hex << allocation;
				}
				
				allocations_[allocation] = size_bytes;
				
				host_addr_t address;
				GetMemoryModel().LockRegion(allocation, pages * archsim::RegionArch::PageSize, address);
				bzero(address, pages * archsim::RegionArch::PageSize);
				
				RB[B] = allocation;
				break;
			}	
			case 2: // abandon
			{
				uint32_t allocation = RB[C];
				uint32_t size = allocations_.at(allocation);
			
				allocations_.erase(allocation);
				free_allocations_[archsim::RegionArch::PageCountOf(size)].push_back(allocation);
				
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
					uint32_t size_bytes = allocations_.at(RB[B]) * 1024;
					uint32_t size_pages = archsim::RegionArch::PageCountOf(size_bytes) * archsim::RegionArch::PageSize;

					GetMemoryModel().GetMappingManager()->UnmapRegion(0, allocations_.at(0));
					GetMemoryModel().GetMappingManager()->MapRegion(0, size_pages, archsim::abi::memory::RegFlagReadWriteExecute, "Program");
					
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
			
			case 6: // self modifying code detected
			{
				uint32_t &EP = *(uint32_t*)GetBootCore()->GetRegisterDescriptor("EP").DataStart;
				
				uint32_t size_bytes = allocations_.at(EP) * 4096;
				uint32_t size_pages = archsim::RegionArch::PageCountOf(size_bytes) * archsim::RegionArch::PageSize;

				GetMemoryModel().GetMappingManager()->UnmapRegion(0, allocations_.at(0));
				GetMemoryModel().GetMappingManager()->MapRegion(0, size_pages, archsim::abi::memory::RegFlagReadWriteExecute, "Program");

				host_addr_t src, dest;
				GetMemoryModel().LockRegion(EP, size_bytes, src);
				GetMemoryModel().LockRegion(0,  size_bytes, dest);

				memcpy(dest, src, size_bytes);
				
				GetSystem().GetProfileManager().Invalidate();
				GetSystem().GetPubSub().Publish(PubSubType::FlushTranslations, (void*)0);
				break;
			}
			case 7: // execution platter modified (copy on write)
			{
				// memcpy the current EP to array 0
				uint32_t &EP = *(uint32_t*)GetBootCore()->GetRegisterDescriptor("EP").DataStart;
				
				uint32_t size_bytes = allocations_.at(EP) * 4096;
				uint32_t size_pages = archsim::RegionArch::PageCountOf(size_bytes) * archsim::RegionArch::PageSize;

				GetMemoryModel().GetMappingManager()->UnmapRegion(0, allocations_.at(0));
				GetMemoryModel().GetMappingManager()->MapRegion(0, size_pages, archsim::abi::memory::RegFlagReadWriteExecute, "Program");

				host_addr_t src, dest;
				GetMemoryModel().LockRegion(EP, size_bytes, src);
				GetMemoryModel().LockRegion(0,  size_bytes, dest);

				memcpy(dest, src, size_bytes);
				
				// update the EF and current EP
				uint32_t &EF = *(uint32_t*)GetBootCore()->GetRegisterDescriptor("EF").DataStart;
				EF -= EP;
				EP = 0;
				
				GetSystem().GetProfileManager().Invalidate();
				GetSystem().GetPubSub().Publish(PubSubType::FlushTranslations, (void*)0);
				
				// retry the instruction
				
				return AbortInstruction;
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
		
		allocations_[0] = bytes.size();
		
		GetMemoryModel().GetMappingManager()->MapRegion(0, max_allocation_bytes_block_, archsim::abi::memory::RegFlagReadWriteExecute, "Program");
		GetMemoryModel().WriteN(0, bytes.data(), bytes.size());
		
		GetMemoryModel().GetMappingManager()->MapRegion(0x80000000, 0x80000000, archsim::abi::memory::RegFlagReadWrite, "Data");
		
		GetBootCore()->write_pc(0);
		next_allocation_ = 0x80000000;
		
		serial_port_ = new archsim::abi::devices::ConsoleSerialPort();
		serial_port_->Open();
		
		return true;
	}

private:
	std::map<uint32_t, uint32_t> allocations_;
	
	// Map of region size (in pages) to free allocations
	std::map<uint32_t, std::vector<uint32_t>> free_allocations_;
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