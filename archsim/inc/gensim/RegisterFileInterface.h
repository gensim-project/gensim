/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RegisterFileInterface.h
 * Author: harry
 *
 * Created on 10 April 2018, 13:50
 */

#ifndef REGISTERFILEINTERFACE_H
#define REGISTERFILEINTERFACE_H

#include <cassert>
#include <cstdint>
#include <unordered_map>

namespace archsim {
		
		class RegisterFileEntryInterface {
		public:
			RegisterFileEntryInterface(const std::string &name, uint32_t id, size_t offset, size_t size_in_elements, size_t element_stride) : name_(name), id_(id), offset_(offset), size_in_elements_(size), element_stride_(element_stride) {}
			
			size_t GetOffset() { return offset_; }
			size_t GetElementCount() { return size_in_elements_; }
			size_t GetSizeInBytes() { return size_in_elements_ * element_stride_; }
			intptr_t GetIndexedPointer(uint32_t index) { assert(index < size_in_elements_); return offset_ + (element_stride_ * index); }
			
			const std::string GetName() const { return name_; }
			uint32_t GetId() const { return id_; }
			
		private:
			std::string name_;
			uint32_t id_;
			
			size_t offset_;
			size_t size_in_elements_;
			
			size_t element_stride_;
			
		};
		
		class RegisterFileInterface {
		public:
			RegisterFileInterface(size_t size_in_bytes, std::initializer_list<RegisterFileEntryInterface> &entries);
			
			bool HasBank(const std::string &name) { return bank_interfaces_.count(name); }
			RegisterFileEntryInterface &GetBank(const std::string &name) { return *bank_interfaces_.at(name); }
			RegisterFileEntryInterface &GetBank(uint32_t bank_id) { return *bank_interfaces_id_.at(bank_id); }
			
			size_t GetSizeInBytes() { return size_in_bytes_; }
			
			void AddEntry(const RegisterFileEntryInterface &entry);
			
		private:
			std::unordered_map<std::string, RegisterFileEntryInterface*> bank_interfaces_;
			std::unordered_map<uint32_t, RegisterFileEntryInterface*> bank_interfaces_id_;
			
			size_t size_in_bytes_;
		};
}

#endif /* REGISTERFILEINTERFACE_H */

