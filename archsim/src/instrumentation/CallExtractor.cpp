/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * CallExtractor.cpp
 *
 *  Created on: 26 Jun 2015
 *      Author: harry
 */

#include "instrumentation/CallExtractor.h"

struct insn_exec_data {
	uint32_t pc;
	uint32_t type;
};

static void callback(PubSubType::PubSubType type, void *ctx, const void *data)
{
	CallExtractor *extractor = (CallExtractor*)ctx;

	switch(type) {
		case PubSubType::FunctionCall:
			extractor->Call((const archsim::abi::BinarySymbol*)data);
			return;
		case PubSubType::FunctionReturnFrom:
			extractor->Return();
			return;
		case PubSubType::InstructionExecute: {
			const struct insn_exec_data *d = (const struct insn_exec_data *)data;
			extractor->InstructionReference(d->type);
			return;
		}
		default:
			return;
	}
}

CallExtractor::CallExtractor(System &sys) : system(sys)
{
	sys.GetPubSub().Subscribe(PubSubType::FunctionCall, callback, this);
	sys.GetPubSub().Subscribe(PubSubType::FunctionReturnFrom, callback, this);

	sys.GetPubSub().Subscribe(PubSubType::InstructionExecute, callback, this);

}

void CallExtractor::InstructionReference(uint32_t insn_type)
{
	for(auto fn : call_stack) {
		stats[fn][insn_type]++;
	}
}

void CallExtractor::Dump(std::ostream &str)
{
	for(auto &sym : stats) {
		str << sym.first->Name << std::endl;
		for(auto &insns : sym.second) {
			str << "\t" << insns.first << "\t" << insns.second << std::endl;
		}
	}
}

void CallExtractor::Call(const archsim::abi::BinarySymbol *callee)
{
	call_stack.push_back(callee);
}

void CallExtractor::Return()
{
	if(call_stack.empty()) return;
	call_stack.pop_back();
}
