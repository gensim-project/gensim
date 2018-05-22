/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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
