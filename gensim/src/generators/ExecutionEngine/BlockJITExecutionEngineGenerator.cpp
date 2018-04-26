/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "generators/ExecutionEngine/BlockJITExecutionEngineGenerator.h"
#include "arch/ArchDescription.h"

using namespace gensim::generator;

BlockJITExecutionEngineGenerator::BlockJITExecutionEngineGenerator(GenerationManager &man) : EEGenerator(man, "blockjit")
{
	man.AddModuleEntry(ModuleEntry("EE", "gensim::" + man.GetArch().Name + "::BlockJITEE", "ee_blockjit.h", ModuleEntryType::ExecutionEngine));
}


bool BlockJITExecutionEngineGenerator::GenerateHeader(util::cppformatstream& str) const
{
	str << "#include <core/execution/BlockJITExecutionEngine.h>\n";
	
	str << "namespace gensim {";
	str << "namespace " << Manager.GetArch().Name << "{";
	
	str << "class BlockJITEE : public archsim::core::execution::BlockJITExecutionEngine {";
	
	str << "};";
	
	str << "}";
	str << "}";
	
	return true;
}

bool BlockJITExecutionEngineGenerator::GenerateSource(util::cppformatstream& str) const
{
	return true;
}

DEFINE_COMPONENT(BlockJITExecutionEngineGenerator, ee_blockjit);