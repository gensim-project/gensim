/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/ExecutionEngineFactory.h"

#include "core/execution/InterpreterExecutionEngine.h"
#include "core/execution/BlockJITExecutionEngine.h"
#include "core/execution/BlockLLVMExecutionEngine.h"

using namespace archsim::core::execution;

ExecutionEngineFactory::ExecutionEngineFactory() {
	factories_.insert({10, InterpreterExecutionEngine::Factory});
	factories_.insert({100, BlockJITExecutionEngine::Factory});
	factories_.insert({200, BlockLLVMExecutionEngine::Factory});
}

ExecutionEngine *ExecutionEngineFactory::Get(const archsim::module::ModuleInfo *module, const std::string &cpu_prefix) {
	for(auto i : factories_) {
		auto result = i.second(module, cpu_prefix);
		if(result) {
			return result;
		}
	}
	
	return nullptr;
}
