/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "arch/MemoryInterfaceDescription.h"

using namespace gensim::arch;

MemoryInterfacesDescription::MemoryInterfacesDescription()
{
}

MemoryInterfaceDescription::MemoryInterfaceDescription(const std::string& name, uint64_t address_width_bytes, uint64_t word_width_bytes, bool big_endian, uint32_t id) : name_(name), address_width_bytes_(address_width_bytes), data_width_bytes_(word_width_bytes), is_big_endian_(big_endian), id_(id)
{

}

bool MemoryInterfacesDescription::Resolve(DiagnosticContext &diag)
{
	if(interfaces_.empty()) {
		diag.Error("No memory interfaces were defined.");
		return false;
	}
	if(fetch_interface_name_ == "") {
		diag.Error("No fetch interface was defined");
		return false;
	}
	return true;
}

void MemoryInterfacesDescription::AddInterface(const MemoryInterfaceDescription &interface)
{
	id_to_name_.push_back(interface.GetName());
	interfaces_.insert({interface.GetName(), interface});
}
