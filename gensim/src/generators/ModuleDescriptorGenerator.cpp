/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "generators/ModuleDescriptorGenerator.h"
#include "arch/ArchDescription.h"

using namespace gensim::generator;

ModuleDescriptorGenerator::ModuleDescriptorGenerator(GenerationManager& man) : GenerationComponent(man, "module")
{

}


bool ModuleDescriptorGenerator::Generate() const
{
	util::cppformatstream stream;

	stream << "#include <module/Module.h>\n";

	for(auto entry : Manager.GetModuleEntries()) {
		stream << "#include \"" << entry.GetClassHeader() << "\"\n";
	}

	stream << "ARCHSIM_MODULE() {";

	stream << "auto module = new archsim::module::ModuleInfo(\"" << Manager.GetArch().Name << "\", \"CPU Module\");";

	for(auto entry : Manager.GetModuleEntries()) {
		stream << "auto entry_" << entry.GetEntryName() << " = new ";

		switch(entry.GetEntryType()) {
			case ModuleEntryType::ExecutionEngine:
				stream << "archsim::module::ModuleExecutionEngineEntry(\"" << entry.GetEntryName() << "\", ARCHSIM_EEFACTORY(" << entry.GetClassName() << "));";
				break;
			case ModuleEntryType::ArchDescriptor:
				stream << "archsim::module::ModuleArchDescriptorEntry(\"" << entry.GetEntryName() << "\", ARCHSIM_ARCHDESCRIPTORFACTORY(" << entry.GetClassName() << "));";
				break;
			case ModuleEntryType::BlockJITTranslator:
				stream << "archsim::module::ModuleBlockJITTranslatorEntry(\"" << entry.GetEntryName() << "\", ARCHSIM_BLOCKJITTRANSLATEFACTORY(" << entry.GetClassName() << "));";
				break;
			case ModuleEntryType::Interpreter:
				stream << "archsim::module::ModuleInterpreterEntry(\"" << entry.GetEntryName() << "\", ARCHSIM_INTERPRETERFACTORY(" << entry.GetClassName() << "));";
				break;
			default:
				UNIMPLEMENTED;
		}

		stream << "module->AddEntry(entry_" << entry.GetEntryName() << ");";
	}

	stream << "return module;";

	stream << "}";
	stream << "ARCHSIM_MODULE_END() {}";

	WriteOutputFile("module.cpp", stream);

	return true;
}

const std::vector<std::string> ModuleDescriptorGenerator::GetSources() const
{
	return {"module.cpp"};
}

std::string ModuleDescriptorGenerator::GetFunction() const
{
	return "module";
}


DEFINE_COMPONENT(ModuleDescriptorGenerator, module);
