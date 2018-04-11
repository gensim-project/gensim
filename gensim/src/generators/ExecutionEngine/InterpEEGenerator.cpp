/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "generators/ExecutionEngine/InterpEEGenerator.h"

using namespace gensim::generator;

bool InterpEEGenerator::GenerateHeader(util::cppformatstream &str) const {
	str << 
		"#include <gensim/ExecutionEngine.h>\n"
		
		"class EE : public archsim::BasicExecutionEngine {"
		"public:"
		"	archsim::ExecutionResult StepThreadSingle(archsim::ThreadInstance *thread);"
		"	archsim::ExecutionResult StepThreadBlock(archsim::ThreadInstance *thread);"
		"};"
		;
	return true;
}

bool InterpEEGenerator::GenerateSource(util::cppformatstream &str) const {
	str <<
		"#include \"ee_interpreter.h\"\n"
		"#include <module/Module.h>\n"
		
		"archsim::ExecutionResult EE::StepThreadSingle(archsim::ThreadInstance *thread) { return archsim::ExecutionResult::Abort; }"
		"archsim::ExecutionResult EE::StepThreadBlock(archsim::ThreadInstance *thread) { return archsim::ExecutionResult::Abort; }"
		
		"ARCHSIM_MODULE() {"
		"  auto module = new archsim::module::ModuleInfo(\"" << Manager.GetArch().Name << "\", \"CPU Module\");"
		"  auto ee_entry = new archsim::module::ModuleExecutionEngineEntry(\"EE\", ARCHSIM_EEFACTORY(EE));"
		
		"  auto arch_entry = new archsim::module::ModuleArchDescriptorEntry(\"ArchDescriptor\", ARCHSIM_ARCHDESCRIPTORFACTORY(ArchDescriptor));"
		
		"	module->AddEntry(ee_entry);"
		"  return module;"
		"}"
		"ARCHSIM_MODULE_END() {}"
		;
		
	return true;
}


InterpEEGenerator::~InterpEEGenerator()
{

}


DEFINE_COMPONENT(InterpEEGenerator, ee_interp)