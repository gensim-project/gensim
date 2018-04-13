/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "generators/ArchDescriptorGenerator.h"

using namespace gensim::generator;

ArchDescriptorGenerator::ArchDescriptorGenerator(GenerationManager& manager) : GenerationComponent(manager, "ArchDescriptor") {
	
}

bool ArchDescriptorGenerator::Generate() const
{
	util::cppformatstream header, source;
	GenerateHeader(header);
	GenerateSource(source);
	
	std::ofstream hfile ((Manager.GetTarget() + "/arch.h").c_str());
	hfile << header.str();
	std::ofstream sfile ((Manager.GetTarget() + "/arch.cpp").c_str());
	sfile << source.str();
}

std::string ArchDescriptorGenerator::GetFunction() const
{
	return "ArchDescriptor";
}

const std::vector<std::string> ArchDescriptorGenerator::GetSources() const
{
	return {"arch.cpp"};
}

bool ArchDescriptorGenerator::GenerateSource(util::cppformatstream &str) const
{
	str << "#include \"arch.h\"\n";
	
	str << "using namespace gensim::" << Manager.GetArch().Name << ";";
	
	auto rfile = Manager.GetArch().GetRegFile();
	std::string entry_list;
	for(auto i : rfile.GetBanks()) {
		str << "static archsim::RegisterFileEntryDescriptor rfd_entry_" << i->ID << "(\"" << i->ID <<"\", " << i->GetIndex() << ", " << i->GetRegFileOffset() << ", " << i->GetRegisterCount() << ", " << i->GetRegisterWidth() << ", " << i->GetRegisterStride() << ");";
		if(entry_list.size()) { entry_list += ", "; }
		entry_list += "rfd_entry_" + i->ID;
	}
	
	for(auto i : rfile.GetSlots()) {
		std::string tag = "";
		if(i->HasTag()) {
			tag = i->GetTag();
		}
		str << "static archsim::RegisterFileEntryDescriptor rfd_entry_" << i->GetID() << "(\"" << i->GetID() <<"\", " << i->GetIndex() << ", " << i->GetRegFileOffset() << ", " << 1 << ", " << i->GetWidth() << ", " << i->GetWidth() << ", \"" << tag << "\");";
		if(entry_list.size()) { entry_list += ", "; }
		entry_list += "rfd_entry_" + i->GetID();
	}
	
	str << "static archsim::RegisterFileDescriptor rfd(" << rfile.GetSize() << ", {" << entry_list << "});";
	
	std::string mem_interface_list;
	for(const auto &mem_interface : Manager.GetArch().GetMemoryInterfaces().GetInterfaces()) {
		str << "static archsim::MemoryInterfaceDescriptor mem_" << mem_interface.second.GetName() << "(\"" << mem_interface.second.GetName() << "\", " << mem_interface.second.GetAddressWidthInBytes() << ", " << mem_interface.second.GetDataWidthInBytes() << ", " << (uint64_t)mem_interface.second.IsBigEndian() << ");";
		
		if(mem_interface_list.size()) {
			mem_interface_list += ", ";
		}
		mem_interface_list += "mem_" + mem_interface.second.GetName();
	}
	
	str << "static archsim::MemoryInterfacesDescriptor misd ({" << mem_interface_list << "},\"" << Manager.GetArch().GetMemoryInterfaces().GetFetchInterface().GetName() << "\");";
	str << "static archsim::FeaturesDescriptor features;";
	
	str << "ArchDescriptor::ArchDescriptor() : archsim::ArchDescriptor(rfd, misd, features) {}";
	
	return true;
}

bool ArchDescriptorGenerator::GenerateHeader(util::cppformatstream &str) const
{	
	str << "#ifndef ARCH_DESC_H\n";
	str << "#define ARCH_DESC_H\n";
	str << "#include <gensim/ArchDescriptor.h>\n";
	str << "#include <gensim/ThreadInstance.h>\n";
	str << "namespace gensim {";
	str << "namespace " << Manager.GetArch().Name << " {";
	str << "class ArchDescriptor : public archsim::ArchDescriptor {";
	str << "public: ArchDescriptor();";
	str << "};";
	
	GenerateThreadInterface(str);
	
	
	str << "} }"; // namespace
		
	str << "\n#endif\n";
	
	return true;
}

bool ArchDescriptorGenerator::GenerateThreadInterface(util::cppformatstream &str) const {
	str << "class ArchInterface {";
	str << "public: ArchInterface(archsim::ThreadInstance *thread) : thread_(thread), reg_file_((char*)thread->GetRegisterFile()) {}";
	
	for(gensim::arch::RegBankViewDescriptor *rbank : Manager.GetArch().GetRegFile().GetBanks()) {
		str << rbank->GetRegisterIRType().GetCType() << " read_register_bank_" << rbank->ID << "(uint32_t idx) { return *(" << rbank->GetRegisterIRType().GetCType() << "*)(reg_file_ + " << rbank->GetRegFileOffset() << " + (idx * " << rbank->GetRegisterStride() << ")); }";
		str << "void write_register_bank_" << rbank->ID << "(uint32_t idx, " << rbank->GetRegisterIRType().GetCType() << " value) { *(" << rbank->GetRegisterIRType().GetCType() << "*)(reg_file_ + " << rbank->GetRegFileOffset() << " + (idx * " << rbank->GetRegisterStride() << ")) = value; }";
	}
	for(gensim::arch::RegSlotViewDescriptor *slot : Manager.GetArch().GetRegFile().GetSlots()) {
		str << slot->GetIRType().GetCType() << " read_register_" << slot->GetID() << "() { return *(" << slot->GetIRType().GetCType() << "*)(reg_file_ + " << slot->GetRegFileOffset() << "); }";
		str << "void write_register_" << slot->GetID() << "(" << slot->GetIRType().GetCType() << " value) { *(" << slot->GetIRType().GetCType() << "*)(reg_file_ + " << slot->GetRegFileOffset() << ") = value; }";
	}
	
	str << "private: "
		"archsim::ThreadInstance *thread_;"
		"char *reg_file_;";
	str << "};";
}



DEFINE_COMPONENT(ArchDescriptorGenerator, arch);
