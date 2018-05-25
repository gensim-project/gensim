/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/LLVMRegionJITExecutionEngine.h"

using namespace archsim::core::execution;

LLVMRegionJITExecutionEngine::LLVMRegionJITExecutionEngine(interpret::Interpreter* interp, gensim::blockjit::BaseBlockJITTranslate* translate) : interpreter_(interp), translator_(translate)
{

}

LLVMRegionJITExecutionEngine::~LLVMRegionJITExecutionEngine()
{

}

ExecutionEngineThreadContext* LLVMRegionJITExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

ExecutionResult LLVMRegionJITExecutionEngine::Execute(ExecutionEngineThreadContext* thread_ctx)
{
	auto thread = thread_ctx->GetThread();
	
	CreateThreadExecutionSafepoint(thread);
	if(thread->GetTraceSource() && thread->GetTraceSource()->IsPacketOpen()) { thread->GetTraceSource()->Trace_End_Insn(); }
	
	while(thread_ctx->GetState() == ExecutionState::Running) {
		if(thread->HasMessage()) {
			auto result = thread->HandleMessage();
			switch(result) {
				case ExecutionResult::Continue:
				case ExecutionResult::Exception:
					break;
				default:
					return result;
			}
		}
		
		auto result = interpreter_->StepBlock(thread);
		switch(result) {
			case ExecutionResult::Continue:
			case ExecutionResult::Exception:
				break;
			default:
				return result;
		}
	}
	return ExecutionResult::Halt;
}

ExecutionEngine* LLVMRegionJITExecutionEngine::Factory(const archsim::module::ModuleInfo* module, const std::string& cpu_prefix)
{
	// need an interpreter and a blockjit translator
	auto interpreter_entry = module->GetEntry<module::ModuleInterpreterEntry>("Interpreter");
	auto blockjit_entry = module->GetEntry<module::ModuleBlockJITTranslatorEntry>("BlockJITTranslator");
	
	if(interpreter_entry == nullptr || blockjit_entry == nullptr) {
		return nullptr;
	}

	return new LLVMRegionJITExecutionEngine(interpreter_entry->Get(), blockjit_entry->Get());
}
