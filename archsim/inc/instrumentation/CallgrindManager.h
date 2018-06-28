/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * CallgrindManager.h
 *
 *  Created on: 4 Jun 2015
 *      Author: harry
 */

#ifndef INC_ABI_CALLGRINDMANAGER_H_
#define INC_ABI_CALLGRINDMANAGER_H_

#include "system.h"
#include "gensim/gensim_processor.h"
#include "abi/EmulationModel.h"
#include "concurrent/ThreadsafeQueue.h"

#include <ostream>
#include <stack>
#include <unordered_map>

struct CallContext {
	uint64_t IRs;
	uint64_t count;
};

class CallgrindThread;

class CallgrindManager
{
public:
	CallgrindManager(System &sys);
	~CallgrindManager();

	void RegisterBlock(uint32_t pc, uint32_t count);
	void ThreadPush(uint32_t pc, uint32_t count);

	void DumpOutput(std::ostream &str);

	uint64_t CountInstructionReferences() const;
private:
	typedef const archsim::abi::BinarySymbol *symbol_t;

	bool IsCall();
	bool IsReturn();

	typedef std::unordered_map<symbol_t, CallContext> call_inner_map_t;

	std::unordered_map<symbol_t, uint64_t> fn_references;
	std::unordered_map<symbol_t, call_inner_map_t > calls;

	call_inner_map_t *curr_call_map;

	std::deque<std::pair<symbol_t, uint64_t>> call_stack;

	System& system;
	const archsim::abi::BinarySymbol *current_symbol;

	uint32_t pc;

	uint32_t prev_type, this_type;
	uint64_t insn_count;
	uint64_t *curr_fn_ref;

	CallgrindThread *Thread;

	bool resync_mode;

	bool detect_calls;

	void Call();
	void Return();
	void Resync();
};

#endif /* INC_ABI_CALLGRINDMANAGER_H_ */
