/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/passes/LexicographicallyOrderBlocksPass.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>

#include <map>

using namespace archsim::translate::translate_llvm;

static bool OrderBlock(int order, llvm::BasicBlock *block, std::map<llvm::BasicBlock*, int> &ordering)
{
	if(ordering.count(block)) {
		return true;
	}
	ordering[block] = order;

	auto terminator = block->getTerminator();
	for(int i = 0; i < terminator->getNumSuccessors(); ++i) {
		auto successor = terminator->getSuccessor(i);
		OrderBlock(order+1, successor, ordering);
	}

	return true;
}

LexicographicallyOrderBlocksPass::LexicographicallyOrderBlocksPass() : FunctionPass(pid)
{

}

char LexicographicallyOrderBlocksPass::pid;


bool LexicographicallyOrderBlocksPass::runOnFunction(llvm::Function& F)
{
	std::map<llvm::BasicBlock*, int> order;
	OrderBlock(0, &F.getEntryBlock(), order);

	std::map<int, std::vector<llvm::BasicBlock*>> rev_order;
	for(auto i : order) {
		rev_order[i.second].push_back(i.first);
	}

	std::vector<llvm::BasicBlock*> final_order;
	for(auto i : rev_order) {
		final_order.insert(final_order.end(), i.second.begin(), i.second.end());
	}

	// todo: actually reorder blocks

	return true;
}
