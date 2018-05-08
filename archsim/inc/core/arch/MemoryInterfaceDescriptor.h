/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ArchDescriptor.h
 * Author: harry
 *
 * Created on 10 April 2018, 15:23
 */

#ifndef MEMORYINTERFACEDESCRIPTOR_H
#define MEMORYINTERFACEDESCRIPTOR_H

#include "abi/Address.h"

#include <functional>
#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace archsim {

	class MemoryInterfaceDescriptor {
	public:
		MemoryInterfaceDescriptor(const std::string &name, uint64_t address_width_bytes, uint64_t data_width_bytes, bool big_endian, uint32_t id);

		const std::string &GetName() const { return name_; }
		uint64_t GetAddressWidthInBytes() const { return address_width_bytes_; }
		uint64_t GetDataWidthInBytes() const { return data_width_bytes_; }
		bool IsBigEndian() const { return is_big_endian_; }
		uint32_t GetID() const { return id_; }

	private:
		std::string name_;
		uint64_t address_width_bytes_;
		uint64_t data_width_bytes_;
		bool is_big_endian_;
		uint32_t id_;
	};

	class MemoryInterfacesDescriptor {
	public:
		using memory_interface_descriptor_collection_t = std::unordered_map<std::string, MemoryInterfaceDescriptor>;

		MemoryInterfacesDescriptor(const std::initializer_list<MemoryInterfaceDescriptor>& interfaces, const std::string& fetch_interface_id);

		const memory_interface_descriptor_collection_t &GetInterfaces() const { return interfaces_; }
		const MemoryInterfaceDescriptor &GetFetchInterface() const { return interfaces_.at(fetch_interface_name_); }
	private:
		memory_interface_descriptor_collection_t interfaces_;
		std::string fetch_interface_name_;
	};

}

#endif /* ARCHDESCRIPTOR_H */

