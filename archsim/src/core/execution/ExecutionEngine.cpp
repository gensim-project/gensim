/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "core/execution/ExecutionEngine.h"
#include "core/thread/ThreadInstance.h"

#include <cassert>

using namespace archsim;
using namespace archsim::core::execution;
using namespace archsim::core::thread;

DeclareLogContext(LogExecutionEngine, "ExecutionEngine");

void ExecutionEngineThreadContext::Execute()
{
	engine_->Execute(this);
}

void ExecutionEngineThreadContext::Start()
{
	std::lock_guard<std::mutex> lg(lock_);
	state_ = ExecutionState::Running;
	worker_ = std::thread([this]() {
		Execute();
	});
}

void ExecutionEngineThreadContext::Suspend()
{
	{
		std::lock_guard<std::mutex> lg(lock_);
		state_ = ExecutionState::Suspending;
		thread_->SendMessage(ThreadMessage::Halt);
	}

	Join();
	state_ = ExecutionState::Ready;
}

void ExecutionEngineThreadContext::Halt()
{
	{
		std::lock_guard<std::mutex> lg(lock_);
		state_ = ExecutionState::Halting;
		thread_->SendMessage(ThreadMessage::Halt);
	}

	Join();
	state_ = ExecutionState::Halted;
}

void ExecutionEngineThreadContext::Join()
{
	if(state_ == ExecutionState::Running) {
		worker_.join();
		state_ = ExecutionState::Ready;
	}
}

ExecutionEngineThreadContext::ExecutionEngineThreadContext(ExecutionEngine *engine, ThreadInstance *thread) : worker_(), engine_(engine), thread_(thread)
{

}

ExecutionEngineThreadContext::~ExecutionEngineThreadContext()
{

}


ExecutionEngine::ExecutionEngine() : state_(ExecutionState::Ready), trace_sink_(nullptr)
{

}

void ExecutionEngine::AttachThread(thread::ThreadInstance* thread)
{
	std::lock_guard<std::mutex> lg(lock_);

	if(thread_contexts_.count(thread)) {
		throw std::logic_error("Thread already attached to this engine");
	}

	if(GetTraceSink()) {
		auto source = new libtrace::TraceSource(1024);
		source->SetSink(GetTraceSink());
		thread->SetTraceSource(source);
	}

	thread_contexts_[thread] = GetNewContext(thread);
	threads_.insert(thread);

	if(state_ == ExecutionState::Running) {
		thread_contexts_[thread]->Start();
	}
}

void ExecutionEngine::DetachThread(thread::ThreadInstance* thread)
{
	std::lock_guard<std::mutex> lg(lock_);
	if(state_ == ExecutionState::Running) {
		throw std::logic_error("Cannot detach a thread while engine is running");
	}

	if(!thread_contexts_.count(thread)) {
		throw std::logic_error("Thread not attached to this engine");
	}

	auto context = GetContext(thread);
	delete context;
	thread_contexts_.erase(thread);
	threads_.erase(thread);
}

ExecutionEngineThreadContext *ExecutionEngine::GetContext(thread::ThreadInstance* thread)
{
	return thread_contexts_.at(thread);
}

void ExecutionEngine::Start()
{
	std::lock_guard<std::mutex> lg(lock_);

	state_ = ExecutionState::Running;

	for(auto i : thread_contexts_) {
		i.second->Start();
	}
}

void ExecutionEngine::Suspend()
{
	std::lock_guard<std::mutex> lg(lock_);

	state_ = ExecutionState::Suspending;

	for(auto i : thread_contexts_) {
		i.second->Suspend();
	}

	state_ = ExecutionState::Ready;
}

void ExecutionEngine::Join()
{
	for(auto i : thread_contexts_) {
		i.second->Join();
	}
}

void ExecutionEngine::Halt()
{
	std::lock_guard<std::mutex> lg(lock_);

	state_ = ExecutionState::Halting;
	for(auto i : thread_contexts_) {
		i.second->Halt();
	}
	state_ = ExecutionState::Halted;
}
