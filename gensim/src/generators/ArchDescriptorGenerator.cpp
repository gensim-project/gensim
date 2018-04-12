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
	GenerateHeader();
	GenerateSource();
}

std::string ArchDescriptorGenerator::GetFunction() const
{
	return "ArchDescriptor";
}

const std::vector<std::string> ArchDescriptorGenerator::GetSources() const
{
	return {"arch.cpp"};
}

bool ArchDescriptorGenerator::GenerateSource() const
{
	std::ofstream str((Manager.GetTarget() + "/arch.cpp").c_str());
	str << "#include \"arch.h\"\n";
	
	auto rfile = Manager.GetArch().GetRegFile();
	
	str << "archsim::RegisterFileDescriptor rfd(" << rfile.GetSize() << ", {});";
	str << "archsim::MemoryInterfacesDescriptor misd;";
	str << "archsim::FeaturesDescriptor features;";
	
	str << "ArchDescriptor::ArchDescriptor() : archsim::ArchDescriptor(rfd, misd, features) {}";
	
	return true;
}

bool ArchDescriptorGenerator::GenerateHeader() const
{
	std::ofstream str((Manager.GetTarget() + "/arch.h").c_str());
	
	str << "#ifndef ARCH_DESC_H\n";
	str << "#define ARCH_DESC_H\n";
	str << "#include <gensim/ArchDescriptor.h>\n";
	str << "class ArchDescriptor : public archsim::ArchDescriptor {";
	str << "public: ArchDescriptor();";
	str << "};";
	
	str << "\n#endif\n";
	
	return true;
}



DEFINE_COMPONENT(ArchDescriptorGenerator, arch);
