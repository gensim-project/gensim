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

#ifndef ARCHDESCRIPTOR_H
#define ARCHDESCRIPTOR_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace archsim {
		class RegisterFileEntryDescriptor {
		public:
			RegisterFileEntryDescriptor(const std::string &name, uint32_t id, uint32_t offset, uint32_t entry_count, uint32_t entry_size, uint32_t entry_stride, const std::string tag = "");
			
			uint64_t GetOffset() const { return offset_; }
			uint64_t GetEntryCount() const { return entry_count_; }
			uint64_t GetEntrySize() const { return entry_size_; }
			uint64_t GetEntryStride() const { return entry_stride_; }
			
			const std::string &GetTag() const { return tag_; }
			
			const std::string &GetName() const { return name_; }
			
		private:
			std::string name_;
			std::string tag_;
			uint32_t id_;
			uint64_t offset_;
			uint64_t entry_count_;
			uint64_t entry_size_;
			uint64_t entry_stride_;
			
		};
		class RegisterFileDescriptor {
		public:
			using register_file_entry_collection_t = std::unordered_map<std::string, RegisterFileEntryDescriptor>;
			
			RegisterFileDescriptor(uint64_t total_size, const std::initializer_list<RegisterFileEntryDescriptor> &entries);
			uint64_t GetSize() const { return total_size_in_bytes_; }
			
			const register_file_entry_collection_t &GetEntries() const { return entries_; }
			
			const RegisterFileEntryDescriptor &GetTaggedEntry(const std::string &tag) const { 
				if(tagged_entries_.count(tag) == 0) {
					throw std::logic_error("Unknown tag \"" + tag + "\"");
				}
				return tagged_entries_.at(tag); 
			}
			
		private:
			uint64_t total_size_in_bytes_;
			register_file_entry_collection_t entries_;
			register_file_entry_collection_t tagged_entries_;
		};
		
		class MemoryInterfaceDescriptor {
		public:
			MemoryInterfaceDescriptor(const std::string &name, uint64_t address_width_bytes, uint64_t data_width_bytes, bool big_endian);
			
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
		
		class FeaturesDescriptor {
			
		};
		
		/**
		 * This class contains all of the metadata about a specific architecture
		 * e.g. the layout of the register file, information about available
		 * memory interfaces, features, etc.
		 */
		class ArchDescriptor {
		public:
			ArchDescriptor(const RegisterFileDescriptor &rf, const MemoryInterfacesDescriptor &mem, const FeaturesDescriptor &f);
			
			const RegisterFileDescriptor &GetRegisterFileDescriptor() const { return register_file_; }
			const MemoryInterfacesDescriptor &GetMemoryInterfaceDescriptor() const { return mem_interfaces_; }
			const FeaturesDescriptor &GetFeaturesDescriptor() const { return features_; }
			
		private:
			RegisterFileDescriptor register_file_;
			MemoryInterfacesDescriptor mem_interfaces_;
			FeaturesDescriptor features_;
		};
}

#endif /* ARCHDESCRIPTOR_H */

