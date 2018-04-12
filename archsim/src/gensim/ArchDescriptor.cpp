/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/ArchDescriptor.h"
#include "define.h"

using namespace archsim;

RegisterFileEntryDescriptor::RegisterFileEntryDescriptor(const std::string& name, uint32_t id, uint32_t offset, uint32_t entry_count, uint32_t entry_size, uint32_t entry_stride) : name_(name), id_(id), offset_(offset), entry_count_(entry_count), entry_size_(entry_size), entry_stride_(entry_stride)
{

}

RegisterFileDescriptor::RegisterFileDescriptor(uint64_t total_size, const std::initializer_list<RegisterFileEntryDescriptor>& entries) : total_size_in_bytes_(total_size)
{
	for(auto &i : entries) {
		entries_.insert({i.GetName(), i});
	}
}


ArchDescriptor::ArchDescriptor(const RegisterFileDescriptor& rf, const MemoryInterfacesDescriptor& mem, const FeaturesDescriptor& f) : register_file_(rf), mem_interfaces_(mem), features_(f)
{

}

