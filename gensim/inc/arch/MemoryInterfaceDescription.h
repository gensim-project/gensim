/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MemoryInterfaceDescription.h
 * Author: harry
 *
 * Created on 12 April 2018, 15:52
 */

#ifndef MEMORYINTERFACEDESCRIPTION_H
#define MEMORYINTERFACEDESCRIPTION_H

#include <unordered_map>
#include <string>

namespace gensim {
	namespace arch {

		class MemoryInterfaceDescription {
		public:
			MemoryInterfaceDescription(const std::string &name, uint64_t address_width_bytes, uint64_t word_width_bytes, bool big_endian);
			
			const std::string &GetName() const { return name_; }
			uint64_t GetAddressWidthInBytes() const { return address_width_bytes_; }
			uint64_t GetDataWidthInBytes() const { return data_width_bytes_; }
			bool IsBigEndian() const { return is_big_endian_; }
			
		private:
			std::string name_;
			uint64_t address_width_bytes_;
			uint64_t data_width_bytes_;
			bool is_big_endian_;
		};
		
		class MemoryInterfacesDescription {
		public:
			using memory_interface_description_collection_t = std::unordered_map<std::string, MemoryInterfaceDescription>;
			
			MemoryInterfacesDescription();
			MemoryInterfacesDescription(const std::initializer_list<MemoryInterfaceDescription> &interfaces, const std::string &fetch_interface_id);
			
			void AddInterface(const MemoryInterfaceDescription &interface) { interfaces_.insert({interface.GetName(), interface}); }
			void SetFetchInterface(const std::string &name) { fetch_interface_name_ = name; }
			
			const memory_interface_description_collection_t &GetInterfaces() const { return interfaces_; }
			const MemoryInterfaceDescription &GetFetchInterface() const { return interfaces_.at(fetch_interface_name_); }
			
		private:
			memory_interface_description_collection_t interfaces_;
			std::string fetch_interface_name_;
		};
	}
}

#endif /* MEMORYINTERFACEDESCRIPTION_H */

