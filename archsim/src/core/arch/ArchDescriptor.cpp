/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/arch/ArchDescriptor.h"
#include "define.h"

using namespace archsim;

RegisterFileEntryDescriptor::RegisterFileEntryDescriptor(const std::string& name, uint32_t id, uint32_t offset, uint32_t entry_count, uint32_t entry_size, uint32_t entry_stride, const std::string tag) : name_(name), id_(id), offset_(offset), entry_count_(entry_count), entry_size_(entry_size), entry_stride_(entry_stride), tag_(tag)
{

}

RegisterFileDescriptor::RegisterFileDescriptor(uint64_t total_size, const std::initializer_list<RegisterFileEntryDescriptor>& entries) : total_size_in_bytes_(total_size)
{
	for(auto &i : entries) {
		entries_.insert({i.GetName(), i});
		if(i.GetTag() != "") {
			tagged_entries_.insert({i.GetTag(), i});
		}
	}
}


ArchDescriptor::ArchDescriptor(const RegisterFileDescriptor& rf, const MemoryInterfacesDescriptor& mem, const FeaturesDescriptor& f, const std::initializer_list<ISADescriptor> &isas) : register_file_(rf), mem_interfaces_(mem), features_(f), isas_(isas)
{
	for(auto isa : isas_) {
		isa_mode_ids_[isa.GetName()] = isa.GetID();
	}
}

MemoryInterfaceDescriptor::MemoryInterfaceDescriptor(const std::string &name, uint64_t address_width_bytes, uint64_t data_width_bytes, bool big_endian, uint32_t id) : name_(name), address_width_bytes_(address_width_bytes), data_width_bytes_(data_width_bytes), is_big_endian_(big_endian), id_(id) {
	
}

MemoryInterfacesDescriptor::MemoryInterfacesDescriptor(const std::initializer_list<MemoryInterfaceDescriptor>& interfaces, const std::string& fetch_interface_id)
{
	for(auto i : interfaces) {
		interfaces_.insert({i.GetName(), i});
	}
	fetch_interface_name_ = fetch_interface_id;
}

ISABehavioursDescriptor::ISABehavioursDescriptor(const std::initializer_list<BehaviourDescriptor> &behaviours)
{
	for(auto &i : behaviours) {
		behaviours_.insert({i.GetName(), i});
	}
}

ISADescriptor::ISADescriptor(const std::string &name, uint32_t id, const DecodeFunction &decoder, const NewDecoderFunction &newdecoder, const NewJumpInfoFunction &newjumpinfo, const NewDTCFunction &newdtc, const ISABehavioursDescriptor &behaviours)
: 
	name_(name), 
	id_(id), 
	decoder_(decoder), 
	new_decoder_(newdecoder),
	new_jump_info_(newjumpinfo),
	new_dtc_(newdtc),
	behaviours_(behaviours)
{
	
}