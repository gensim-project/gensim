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

	thread->GetStateBlock().AddBlock("page_cache", 8);
	thread->GetStateBlock().SetEntry("page_cache", &PageCache.Cache[0]);
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

	archsim::util::CounterTimerContext self_timer(thread->GetMetrics().SelfRuntime);

	CreateThreadExecutionSafepoint(thread);
	if(thread->GetTraceSource() && thread->GetTraceSource()->IsPacketOpen()) {
		thread->GetTraceSource()->Trace_End_Insn();
	}

	while(thread_ctx->GetState() == ExecutionState::Running) {
		ctx->Iterations = 10000;
		while(ctx->Iterations-- > 0) {
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

			LC_DEBUG3(LogRegionJIT) << "Looking for region for V" << virt_pc;

			auto txln = thread->GetFetchMI().PerformTranslation(virt_pc, phys_pc, false, true, false);
			auto &region = ctx->TxlnMgr.GetRegion(phys_pc);
			ctx->CurrentRegion = &region;

			LC_DEBUG3(LogRegionJIT) << "Looking for region P" << region.GetPhysicalBaseAddress();

			if(region.HasTranslations()) {
				LC_DEBUG3(LogRegionJIT) << "Region " << region.GetPhysicalBaseAddress() << " has translations";

				LC_DEBUG3(LogRegionJIT) << "Region " << region.GetPhysicalBaseAddress() << " has virtual images:";
				for(auto i : region.virtual_images) {
					LC_DEBUG3(LogRegionJIT) << Address(i);
				}

				if(region.txln != nullptr && region.virtual_images.count(virt_pc.PageBase()) && region.txln->ContainsBlock(virt_pc.PageOffset())) {
					LC_DEBUG3(LogRegionJIT) << "Translation found for current block";
					captive::shared::block_txln_fn fn;
					region.txln->Install(&fn);
					ctx->PageCache.InsertEntry(virt_pc.PageBase(), fn);

					if(archsim::options::Verbose) {
						thread->GetMetrics().JITTime.Start();
					}
					region.txln->Execute(thread);
					if(archsim::options::Verbose) {
						thread->GetMetrics().JITTime.Stop();
					}
					continue;
				} else {
					LC_DEBUG3(LogRegionJIT) << "Translation NOT found for current block";
				}
			}

			if(archsim::options::Verbose) {
				thread->GetMetrics().InterpretTime.Start();
			}
			auto interpret_result = EpochInterpret(ctx);
			if(archsim::options::Verbose) {
				thread->GetMetrics().InterpretTime.Stop();
			}
			switch(interpret_result) {
				case ExecutionResult::Continue:
				case ExecutionResult::Exception:
					break;
				default:
					return interpret_result;
			}

		}
		ctx->TxlnMgr.Profile(thread);
		ctx->TxlnMgr.RegisterCompletedTranslations();

	}
	return ExecutionResult::Halt;
}

ExecutionResult LLVMRegionJITExecutionEngine::EpochInterpret(LLVMRegionJITExecutionEngineContext *ctx)
{
	auto thread = ctx->GetThread();
	archsim::translate::profile::Region* region = nullptr;

	while(ctx->Iterations-- > 0 && !thread->HasMessage()) {


		auto virt_pc = thread->GetPC();
		if(region == nullptr || !region->virtual_images.count(virt_pc)) {
			if(region != nullptr) {
				region->Release();
			}

			Address phys_pc;
			thread->GetFetchMI().PerformTranslation(virt_pc, phys_pc, false, true, false);
			region = &ctx->TxlnMgr.GetRegion(phys_pc);
			region->Acquire();
		}
		if(region->txln != nullptr && region->txln->ContainsBlock(virt_pc.PageOffset())) {
			return ExecutionResult::Continue;
		}

		region->TraceBlock(thread, virt_pc);
		auto result = interpreter_->StepBlock(ctx);

		switch(result) {
			case ExecutionResult::Continue:
			case ExecutionResult::Exception:
				break;
			default:
				return result;
		}
	}
	return ExecutionResult::Continue;
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
