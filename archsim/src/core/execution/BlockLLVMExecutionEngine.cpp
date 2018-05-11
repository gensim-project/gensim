/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/BlockLLVMExecutionEngine.h"

using namespace archsim::core::execution;

ExecutionEngineThreadContext* BlockLLVMExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

ExecutionResult BlockLLVMExecutionEngine::Execute(ExecutionEngineThreadContext* thread)
{
	archsim::translate::TranslationManager manager;
	archsim::translate::llvm::LLVMTranslationEngine
}
