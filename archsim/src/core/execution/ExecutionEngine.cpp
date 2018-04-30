/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "core/execution/ExecutionEngine.h"
#include "core/thread/ThreadInstance.h"

#include <cassert>

using namespace archsim;
using namespace archsim::core::execution;
using namespace archsim::core::thread;

DeclareLogContext(LogExecutionEngine, "ExecutionEngine");

void execute_thread(BasicExecutionEngine::ExecutionContext *ctx) {
	while(!ctx->should_halt && ctx->last_result == ExecutionResult::Continue) {
		ctx->last_result = ctx->engine->StepThreadBlock(ctx->thread_instance);
		
		assert(ctx->last_result != ExecutionResult::Exception);
	}
}

BasicExecutionEngine::BasicExecutionEngine() : decode_context_(nullptr) {}

void BasicExecutionEngine::StartAsyncThread(ThreadInstance* thread)
{
	ExecutionContext *ctx = new ExecutionContext();
	ctx->should_halt = false;
	ctx->last_result = ExecutionResult::Continue;
	ctx->thread_instance = thread;
	ctx->engine = this;
	
	ctx->thread = new std::thread(execute_thread, ctx);
		
	threads_[thread] = ctx;
}

void BasicExecutionEngine::HaltAsyncThread(ThreadInstance* thread)
{
	threads_.at(thread)->should_halt = true;
	threads_.at(thread)->thread->join();
}

ExecutionResult BasicExecutionEngine::JoinAsyncThread(ThreadInstance* thread)
{
	threads_.at(thread)->thread->join();
}

ExecutionResult BasicExecutionEngine::Execute(ThreadInstance* thread)
{
	ExecutionResult result;
	do {
		result = StepThreadBlock(thread);
	} while(result == ExecutionResult::Continue);
}

ExecutionResult BasicExecutionEngine::StepThreadBlock(ThreadInstance* thread)
{
	archsim::util::CounterTimerContext timer(thread->GetMetrics().SelfRuntime);
	try {
		return ArchStepBlock(thread);
	} catch(ThreadException &exception) {
		if(thread->GetTraceSource()) {
			thread->GetTraceSource()->Trace_End_Insn();
		}
		return ExecutionResult::Exception;
	}
}
ExecutionResult BasicExecutionEngine::StepThreadSingle(ThreadInstance* thread)
{
	archsim::util::CounterTimerContext timer(thread->GetMetrics().SelfRuntime);
	try {
		return ArchStepSingle(thread);
	} catch(ThreadException &exception) {
		if(thread->GetTraceSource()) {
			thread->GetTraceSource()->Trace_End_Insn();
		}
		return ExecutionResult::Exception;
	}
}