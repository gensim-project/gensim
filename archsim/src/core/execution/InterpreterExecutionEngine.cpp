/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/execution/InterpreterExecutionEngine.h"
#include "core/thread/ThreadInstance.h"
#include "core/execution/ExecutionEngineFactory.h"
#include "system.h"

using namespace archsim::core::execution;

InterpreterExecutionEngineThreadContext::InterpreterExecutionEngineThreadContext(ExecutionEngine* engine, thread::ThreadInstance* thread) : ExecutionEngineThreadContext(engine, thread)
{
	auto &isa = thread->GetArch().GetISA(0);
	decode_ctx_ = thread->GetEmulationModel().GetNewDecodeContext(*thread);
	if(!archsim::options::AggressiveCodeInvalidation) {
		decode_ctx_ = new gensim::CachedDecodeContext(thread->GetEmulationModel().GetSystem().GetPubSub(), decode_ctx_, [&isa]() {
			return isa.GetNewDecode();
		});
	}
}

InterpreterExecutionEngineThreadContext::~InterpreterExecutionEngineThreadContext()
{

}


InterpreterExecutionEngine::InterpreterExecutionEngine(archsim::interpret::Interpreter* interpreter) : interpreter_(interpreter)
{

}

ExecutionResult InterpreterExecutionEngine::Execute(ExecutionEngineThreadContext* thread_ctx)
{
	archsim::util::CounterTimerContext self_timer(thread_ctx->GetThread()->GetMetrics().SelfRuntime);

	InterpreterExecutionEngineThreadContext *ieetc = (InterpreterExecutionEngineThreadContext*)thread_ctx;
	auto thread = thread_ctx->GetThread();

	CreateThreadExecutionSafepoint(thread);
	if(thread->GetTraceSource() && thread->GetTraceSource()->IsPacketOpen()) {
		thread->GetTraceSource()->Trace_End_Insn();
	}

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

		auto result = interpreter_->StepBlock(ieetc);
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
	return new InterpreterExecutionEngineThreadContext(this, thread);
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

static archsim::core::execution::ExecutionEngineFactoryRegistration registration("Interpreter", 10, archsim::core::execution::InterpreterExecutionEngine::Factory);
