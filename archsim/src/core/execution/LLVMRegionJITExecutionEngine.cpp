/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/LLVMRegionJITExecutionEngine.h"
#include "core/execution/ExecutionEngineFactory.h"
#include "core/MemoryInterface.h"
#include "system.h"
#include "translate/Translation.h"

using namespace archsim::core::execution;

LLVMRegionJITExecutionEngineContext::LLVMRegionJITExecutionEngineContext(archsim::core::execution::ExecutionEngine* engine, archsim::core::thread::ThreadInstance* thread) : ExecutionEngineThreadContext(engine, thread), TxlnMgr(&thread->GetEmulationModel().GetSystem().GetPubSub())
{
	LLVMRegionJITExecutionEngine *e = (LLVMRegionJITExecutionEngine*)engine;
	
	TxlnMgr.SetManager(thread->GetEmulationModel().GetSystem().GetCodeRegions());
	TxlnMgr.Initialise(e->translator_);
}


LLVMRegionJITExecutionEngine::LLVMRegionJITExecutionEngine(interpret::Interpreter* interp, gensim::blockjit::BaseBlockJITTranslate* translate) : interpreter_(interp), translator_(translate)
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
	
	CreateThreadExecutionSafepoint(thread);
	if(thread->GetTraceSource() && thread->GetTraceSource()->IsPacketOpen()) { thread->GetTraceSource()->Trace_End_Insn(); }
	
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
						return result;
				}
			}

			Address virt_pc = thread->GetPC();
			Address phys_pc(0);
			auto txln = thread->GetFetchMI().PerformTranslation(virt_pc, phys_pc, false, true, false);
			auto &region = ctx->TxlnMgr.GetRegion(phys_pc.Get());

			if(region.HasTranslations()) {
				if(region.txln != nullptr && region.txln->ContainsBlock(virt_pc.GetPageOffset())) {
					region.txln->Execute(thread);
					continue;
				}
			}
			
			region.TraceBlock(thread, virt_pc);
			auto result = interpreter_->StepBlock(thread);

			switch(result) {
				case ExecutionResult::Continue:
				case ExecutionResult::Exception:
					break;
				default:
					return result;
			}
		}
		ctx->TxlnMgr.Profile(thread);
		ctx->TxlnMgr.RegisterCompletedTranslations();
		
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

static ExecutionEngineFactoryRegistration registration("LLVMRegionJIT", 1000, LLVMRegionJITExecutionEngine::Factory);
