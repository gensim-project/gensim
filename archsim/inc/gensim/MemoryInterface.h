/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MemoryInterface.h
 * Author: harry
 *
 * Created on 10 April 2018, 16:10
 */

#ifndef MEMORYINTERFACE_H
#define MEMORYINTERFACE_H

#include "ArchDescriptor.h"
#include "abi/Address.h"
#include "abi/memory/MemoryModel.h"

namespace archsim {
	
	enum class MemoryResult {
		OK,
		Error
	};
	
	class MemoryDevice {
	public:
		
		virtual MemoryResult Read8(Address address, uint8_t &data) = 0;
		virtual MemoryResult Read16(Address address, uint16_t &data) = 0;
		virtual MemoryResult Read32(Address address, uint32_t &data) = 0;
		virtual MemoryResult Read64(Address address, uint64_t &data) = 0;
		
		virtual MemoryResult Write8(Address address, uint8_t data) = 0;
		virtual MemoryResult Write32(Address address, uint32_t data) = 0;
		virtual MemoryResult Write64(Address address, uint64_t data) = 0;
		virtual MemoryResult Write16(Address address, uint16_t data) = 0;
		
	private:
		
	};
	
	class LegacyMemoryInterface : public MemoryDevice {
	public:
		LegacyMemoryInterface(archsim::abi::memory::MemoryModel &mem_model) : mem_model_(mem_model) {}
		
		MemoryResult Read8(Address address, uint8_t& data) override;
		MemoryResult Read16(Address address, uint16_t& data) override;
		MemoryResult Read32(Address address, uint32_t& data) override;
		MemoryResult Read64(Address address, uint64_t& data) override;
		MemoryResult Write8(Address address, uint8_t data) override;
		MemoryResult Write16(Address address, uint16_t data) override;
		MemoryResult Write32(Address address, uint32_t data) override;
		MemoryResult Write64(Address address, uint64_t data) override;

	private:
		archsim::abi::memory::MemoryModel &mem_model_;
	};
	
	/**
	 * This class represents a specific instantiation of a memory master.
	 * This is connected to an underlying device (which may be a bus).
	 */
	class MemoryInterface {
	public:			
		MemoryInterface(const MemoryInterfaceDescriptor &descriptor) : descriptor_(descriptor) {}
		const MemoryInterfaceDescriptor &GetDescriptor() { return descriptor_; }

		MemoryResult Read8 (Address address, uint8_t &data) { return device_->Read8(address, data); } 
		MemoryResult Read16(Address address, uint16_t &data) { return device_->Read16(address, data); } 
		MemoryResult Read32(Address address, uint32_t &data) { return device_->Read32(address, data); }
		MemoryResult Read64(Address address, uint64_t &data) { return device_->Read64(address, data); }
		
		MemoryResult Write8 (Address address, uint8_t data) { return device_->Write8(address, data); } 
		MemoryResult Write16(Address address, uint16_t data) { return device_->Write16(address, data); } 
		MemoryResult Write32(Address address, uint32_t data) { return device_->Write32(address, data); }
		MemoryResult Write64(Address address, uint64_t data) { return device_->Write64(address, data); }

		MemoryResult ReadString(Address address,  char *data, size_t max_size);
		MemoryResult WriteString(Address address, const char *data);
		
		MemoryResult Read(Address address, unsigned char *data, size_t size);
		MemoryResult Write(Address address, const unsigned char *data, size_t size);
		
		void Connect(MemoryDevice &device) { device_ = &device; }

	private:
		const MemoryInterfaceDescriptor &descriptor_;
		MemoryDevice *device_;
	};

}

#endif /* MEMORYINTERFACE_H */

