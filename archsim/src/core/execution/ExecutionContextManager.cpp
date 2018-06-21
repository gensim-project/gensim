/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/execution/ExecutionContextManager.h"

using namespace archsim;
using namespace archsim::core::execution;
using namespace archsim::core::thread;

ExecutionContextManager::ExecutionContextManager() : state_(ExecutionState::Ready), trace_sink_(nullptr)
{

}

void ExecutionContextManager::AddEngine(ExecutionEngine* engine)
{
	engine->SetTraceSink(GetTraceSink());
	contexts_.push_back(engine);
}

void ExecutionContextManager::Start()
{
	for(auto i : contexts_) {
		i->Start();
	}
}

void ExecutionContextManager::Join()
{
	for(auto i : contexts_) {
		i->Join();
	}
}

void ExecutionContextManager::Halt()
{
	for(auto i : contexts_) {
		i->Halt();
	}
}
