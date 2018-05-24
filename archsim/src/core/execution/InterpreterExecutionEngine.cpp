/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/InterpreterExecutionEngine.h"
#include "core/thread/ThreadInstance.h"

using namespace archsim::core::execution;

InterpreterExecutionEngine::InterpreterExecutionEngine(archsim::interpret::Interpreter* interpreter) : interpreter_(interpreter)
{

}

ExecutionResult InterpreterExecutionEngine::Execute(ExecutionEngineThreadContext* thread_ctx)
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

ExecutionEngineThreadContext* InterpreterExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new ExecutionEngineThreadContext(this, thread);
}

ExecutionEngine* InterpreterExecutionEngine::Factory(const archsim::module::ModuleInfo* module, const std::string& cpu_prefix)
{
	std::string entry_name = cpu_prefix + "Interpreter";
	if(!module->HasEntry(entry_name)) {
		return nullptr;
	}
	
	auto interpreter = module->GetEntry<archsim::module::ModuleInterpreterEntry>(entry_name)->Get();
	return new InterpreterExecutionEngine(interpreter);
}
