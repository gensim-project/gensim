/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * CallgrindManager.cpp
 *
 *  Created on: 4 Jun 2015
 *      Author: harry
 */

#include "instrumentation/CallgrindManager.h"
#include "concurrent/Thread.h"

#include <cxxabi.h>

//#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define LOG(...)

struct block_ctx {
	uint32_t pc, count;
};

class CallgrindThread : public archsim::concurrent::Thread
{
public:
	CallgrindThread(CallgrindManager *manager) : Thread("Callgrind Manager"), manager(manager), running(false)
	{

	}

	void Push(uint32_t pc, uint32_t count)
	{
		// Can only push entries while we are running
		assert(running);

		struct block_ctx ctx;
		ctx.pc = pc;
		ctx.count = count;
		queue.push(ctx);
	}

	void Stop()
	{
		running = false;
	}

	virtual void run() override
	{
		running = true;

		// Run while we should be running. If we should stop, empty the queue first.
		while(running || !queue.empty()) {
			if(queue.empty()) pthread_yield();
			while(running && queue.empty()) asm("pause");
			if(!running) break;

			const struct block_ctx &ctx = queue.front();
			manager->RegisterBlock(ctx.pc, ctx.count);
			queue.pop();
		}
	}

private:
	CallgrindManager *manager;
	archsim::concurrent::ThreadsafeQueue<block_ctx, 1024*1024, false> queue;
	volatile bool running;
};

void callback_exec_insn(PubSubType::PubSubType type, void *ctx, const void *data)
{
	assert(type == PubSubType::BlockExecute);
	struct block_ctx *d = (struct block_ctx*)data;
	((CallgrindManager*)ctx)->ThreadPush(d->pc, d->count);
}

CallgrindManager::CallgrindManager(System &sys) : system(sys), current_symbol(NULL), prev_type(-1), this_type(-1), pc(0), resync_mode(0), insn_count(0), detect_calls(true)
{
	system.GetPubSub().Subscribe(PubSubType::BlockExecute, callback_exec_insn, (void*)this);
	Thread = new CallgrindThread(this);
	Thread->start();
}

CallgrindManager::~CallgrindManager()
{
	Thread->Stop();
	Thread->join();
	std::ofstream of ("cg.out");
	DumpOutput(of);
}

void CallgrindManager::ThreadPush(uint32_t pc, uint32_t count)
{
	Thread->Push(pc, count);
}

uint64_t CallgrindManager::CountInstructionReferences() const
{
	return insn_count;
}

std::string demangle(std::string input)
{
	char *realname;
	int status;

	realname = abi::__cxa_demangle(input.c_str(), 0, 0, &status);

	if(status) return input;

	std::string out = std::string(realname);
	free(realname);
	return out;
}

void CallgrindManager::DumpOutput(std::ostream &str)
{
	str << "version: 1" << std::endl;
	str << "creator: Arcsim" << std::endl;
	str << "part: 1" << std::endl;

	str << "positions: line" << std::endl;
	str << "events: Ir" << std::endl;

	str << std::endl;

	assert(detect_calls || calls.empty());

	insn_count = 0;
	for(auto i : system.GetEmulationModel().GetBootCore()->metrics.pc_freq_hist.get_value_map()) {
		if(!current_symbol || !current_symbol->Contains(i.first)) {
			if(!system.GetEmulationModel().LookupSymbol((uint32_t)i.first, false, current_symbol)) current_symbol = NULL;
		}
		if(current_symbol) {
			fn_references[current_symbol] += *i.second;
			insn_count += *i.second;
		}
	}

	str << "summary: " << CountInstructionReferences() << std::endl;

	for(const auto fn : fn_references) {

		str << "fn=" << demangle(fn.first->Name) << std::endl;
		str << "0 " << fn.second << std::endl;

		if(calls.count(fn.first)) {
			for(const auto &call : calls.at(fn.first)) {

				str << "cfn=" << demangle(call.first->Name) << std::endl;

				str << "calls=" << call.second.count << " 0" << std::endl;
				str << "0 " << call.second.IRs << std::endl;
			}
		}

		str << std::endl;
	}

	str << "totals: " << CountInstructionReferences() << std::endl;
}

void CallgrindManager::RegisterBlock(uint32_t pc, uint32_t count)
{
	const archsim::abi::BinarySymbol *prev_sym = current_symbol;

	if(current_symbol == NULL || !current_symbol->Contains(pc)) {
		if(!system.GetEmulationModel().LookupSymbol(pc, false, current_symbol)) {
			current_symbol = NULL;
			LOG("%08x does not have a symbol!\n", pc);
		}
	}

	if(detect_calls) {
		if(current_symbol != NULL && prev_sym != NULL) {
			if(current_symbol && current_symbol != prev_sym) {
				if(resync_mode) {
					Resync();
				} else {

					bool returned = false;
					for(auto i = call_stack.rbegin(); i != call_stack.rend(); ++i) {
						if(i->first == current_symbol) {
							Return();
							returned = true;
							break;
						}
					}

					if(!returned) Call();
				}
			}
		}

		if(prev_sym == NULL && current_symbol != NULL) Call();
	}

	insn_count += count;
}

void CallgrindManager::Call()
{
	LOG("Called %s (%lu) %08x\n", current_symbol->Name.c_str(), call_stack.size(), pc);

	for(auto &i : call_stack) {
		if(i.first == current_symbol) return;
	}

	call_stack.push_back({current_symbol, insn_count});

	if(system.GetPubSub().HasSubscribers(PubSubType::FunctionCall))
		system.GetPubSub().Publish(PubSubType::FunctionCall, current_symbol);
}

void CallgrindManager::Return()
{
	LOG("Return to %s (%lu) %08x\n", current_symbol->Name.c_str(), call_stack.size(), pc);

	// First, try and perform a standard return. The 'top' of the stack is the callee, the next entry down
	// is the return target
	auto rtarget = ++call_stack.rbegin();
	LOG("%s is top of stack\n", rtarget->first->Name.c_str());
	if(rtarget->first == current_symbol) {
		// We can perform a standard return
		auto &callee = call_stack.back();

		auto &ctx = calls[rtarget->first][callee.first];
		ctx.count++;
		ctx.IRs += insn_count - callee.second;

		call_stack.pop_back();

		if(system.GetPubSub().HasSubscribers(PubSubType::FunctionReturnFrom))
			system.GetPubSub().Publish(PubSubType::FunctionReturnFrom, callee.first);

		return;
	}

	// Otherwise, something weird has happened. Go into 'resync' mode
	LOG("%s is not top of stack: attempting to resync\n", current_symbol->Name.c_str());
	resync_mode = 1;
	Resync();
}

void CallgrindManager::Resync()
{
	LOG("Attempting to resync in %s (%lu) %08x\n", current_symbol->Name.c_str(), call_stack.size(), pc);

	// look down the call stack. If we find a symbol matching our current symbol, discard everything above it
	auto found_i = call_stack.rbegin();
	bool found = false;
	for(auto i = call_stack.rbegin(); i != call_stack.rend(); ++i) {
		LOG(" - %s\n", i->first->Name.c_str());
		if(i->first == current_symbol) {
			found = true;
			found_i = i;
			break;
		}
	}

	if(found) {

		while(call_stack.back().first != current_symbol) {
			auto caller = *(++call_stack.rbegin());
			auto callee = *call_stack.rbegin();

			CallContext &ctx = calls[caller.first][callee.first];
			ctx.count++;
			ctx.IRs += insn_count - callee.second;

			call_stack.pop_back();
		}

		resync_mode = 0;
		LOG("Resync'd\n");
	}
}
