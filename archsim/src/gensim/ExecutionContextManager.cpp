/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gensim/ExecutionContextManager.h"

using namespace archsim;

ExecutionContext::ExecutionContext(ArchDescriptor& arch, ExecutionEngine* engine) : arch_(arch), engine_(engine), trace_sink_(nullptr)
{

}

void ExecutionContext::AddThread(ThreadInstance* thread)
{
	local_threads_.push_back(thread);
	
	if(GetTraceSink() != nullptr) {
		auto source = new libtrace::TraceSource(1024*1024);
		source->SetSink(GetTraceSink());
		source->SetAggressiveFlush(true);
		thread->SetTraceSource(source);
	}
}

ExecutionResult ExecutionContext::StepThreadsBlock()
{
	ExecutionResult result = ExecutionResult::Continue;
	
	for(auto thread : local_threads_) {
		auto tresult = engine_->StepThreadBlock(thread);
		if(tresult != result) {
			return tresult;
		}
	}
	return result;
}


ExecutionContextManager::ExecutionContextManager() : trace_sink_(nullptr) {
	
}

void ExecutionContextManager::AddContext(ExecutionContext* ctx)
{
	contexts_.push_back(ctx);
	ctx->SetTraceSink(GetTraceSink());
}

void ExecutionContextManager::StartSync()
{
	while(true) {
		for(auto ctx : contexts_) {
			auto cresult = ctx->StepThreadsBlock();

			if(cresult != ExecutionResult::Continue) {
				return;
			}
		}
	}
}
