/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

#include "DiagnosticContext.h"

namespace gensim {
	namespace arch {

		class MemoryInterfaceDescription {
		public:
			MemoryInterfaceDescription(const std::string &name, uint64_t address_width_bytes, uint64_t word_width_bytes, bool big_endian, uint32_t id);
			
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
		
		class MemoryInterfacesDescription {
		public:
			using memory_interface_description_collection_t = std::unordered_map<std::string, MemoryInterfaceDescription>;
			
			MemoryInterfacesDescription();
			MemoryInterfacesDescription(const std::initializer_list<MemoryInterfaceDescription> &interfaces, const std::string &fetch_interface_id);
			
			void AddInterface(const MemoryInterfaceDescription &interface);
			void SetFetchInterface(const std::string &name) { fetch_interface_name_ = name; }
			
			const memory_interface_description_collection_t &GetInterfaces() const { return interfaces_; }
			const MemoryInterfaceDescription &GetFetchInterface() const { return interfaces_.at(fetch_interface_name_); }
			
			const MemoryInterfaceDescription *GetByID(uint32_t id) const { for(auto &i : interfaces_) { if(i.second.GetID() == id) return &i.second;} return nullptr; }
                        
			bool Resolve(DiagnosticContext &diag);
			
		private:
			std::vector<std::string> id_to_name_;
			memory_interface_description_collection_t interfaces_;
			std::string fetch_interface_name_;
		};
	}
}

#endif /* MEMORYINTERFACEDESCRIPTION_H */

