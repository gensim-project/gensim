/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/execution/LLVMRegionJITExecutionEngine.h"
#include "core/execution/ExecutionEngineFactory.h"
#include "core/MemoryInterface.h"
#include "system.h"
#include "translate/Translation.h"

using namespace archsim::core::execution;
DeclareLogContext(LogRegionJIT, "LLVMRegionJIT");

LLVMRegionJITExecutionEngineContext::LLVMRegionJITExecutionEngineContext(archsim::core::execution::ExecutionEngine* engine, archsim::core::thread::ThreadInstance* thread) : InterpreterExecutionEngineThreadContext(engine, thread), TxlnMgr(&thread->GetEmulationModel().GetSystem().GetPubSub())
{
	LLVMRegionJITExecutionEngine *e = (LLVMRegionJITExecutionEngine*)engine;

	TxlnMgr.SetManager(thread->GetEmulationModel().GetSystem().GetCodeRegions());
	if(!TxlnMgr.Initialise(e->translator_)) {
		throw std::logic_error("");
	}
}


LLVMRegionJITExecutionEngine::LLVMRegionJITExecutionEngine(interpret::Interpreter* interp, gensim::BaseLLVMTranslate* translate) : interpreter_(interp), translator_(translate)
{

}

LLVMRegionJITExecutionEngine::~LLVMRegionJITExecutionEngine()
{

}

ExecutionEngineThreadContext* LLVMRegionJITExecutionEngine::GetNewContext(thread::ThreadInstance* thread)
{
	return new LLVMRegionJITExecutionEngineContext(this, thread);
}

ExecutionResult LLVMRegionJITExecutionEngine::Execute(ExecutionEngineThreadContext* thread_ctx)
{
	LLVMRegionJITExecutionEngineContext *ctx = (LLVMRegionJITExecutionEngineContext*)thread_ctx;
	auto thread = thread_ctx->GetThread();

	thread->GetMetrics().SelfRuntime.Start();

	CreateThreadExecutionSafepoint(thread);
	if(thread->GetTraceSource() && thread->GetTraceSource()->IsPacketOpen()) {
		thread->GetTraceSource()->Trace_End_Insn();
	}

	while(thread_ctx->GetState() == ExecutionState::Running) {
		int iterations = 10000;
		while(iterations--) {
			if(thread->HasMessage()) {
				auto result = thread->HandleMessage();
				switch(result) {
					case ExecutionResult::Continue:
					case ExecutionResult::Exception:
						break;
					default:
						thread->GetMetrics().SelfRuntime.Stop();
						return result;
				}
			}

			Address virt_pc = thread->GetPC();
			Address phys_pc(0);
			auto txln = thread->GetFetchMI().PerformTranslation(virt_pc, phys_pc, false, true, false);
			auto &region = ctx->TxlnMgr.GetRegion(phys_pc);

			LC_DEBUG3(LogRegionJIT) << "Looking for region " << region.GetPhysicalBaseAddress();

			if(region.HasTranslations()) {
				LC_DEBUG3(LogRegionJIT) << "Region " << region.GetPhysicalBaseAddress() << " has translations";
				if(region.txln != nullptr && region.txln->ContainsBlock(virt_pc.PageOffset())) {
					LC_DEBUG3(LogRegionJIT) << "Translation found for current block";
					thread->GetMetrics().JITTime.Start();
					region.txln->Execute(thread);
					thread->GetMetrics().JITTime.Stop();
					continue;
				} else {
					LC_DEBUG3(LogRegionJIT) << "Translation NOT found for current block";
				}
			}

			region.TraceBlock(thread, virt_pc);
			auto result = interpreter_->StepBlock(ctx);

			switch(result) {
				case ExecutionResult::Continue:
				case ExecutionResult::Exception:
					break;
				default:
					thread->GetMetrics().SelfRuntime.Stop();
					return result;
			}
		}
		ctx->TxlnMgr.Profile(thread);
		ctx->TxlnMgr.RegisterCompletedTranslations();

	}
	thread->GetMetrics().SelfRuntime.Stop();
	return ExecutionResult::Halt;
}

ExecutionEngine* LLVMRegionJITExecutionEngine::Factory(const archsim::module::ModuleInfo* module, const std::string& cpu_prefix)
{
	// need an interpreter and a blockjit translator
	auto interpreter_entry = module->GetEntry<module::ModuleInterpreterEntry>("Interpreter");
	auto blockjit_entry = module->GetEntry<module::ModuleLLVMTranslatorEntry>("LLVMTranslator");

	if(interpreter_entry == nullptr || blockjit_entry == nullptr) {
		return nullptr;
	}

	return new LLVMRegionJITExecutionEngine(interpreter_entry->Get(), blockjit_entry->Get());
}

static ExecutionEngineFactoryRegistration registration("LLVMRegionJIT", 1000, LLVMRegionJITExecutionEngine::Factory);
