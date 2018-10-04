/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   InterruptCheckingScheme.cpp
 * Author: s0457958
 *
 * Created on 05 August 2014, 14:27
 */
#include "translate/interrupts/InterruptCheckingScheme.h"
#include "translate/TranslationWorkUnit.h"
#include "util/ComponentManager.h"

using archsim::Address;
using namespace archsim::translate;
using namespace archsim::translate::interrupts;

DefineComponentType(InterruptCheckingScheme);
RegisterComponent(InterruptCheckingScheme, NoneInterruptCheckingScheme, "none", "No Interrupt Checking in the JIT");
RegisterComponent(InterruptCheckingScheme, FullInterruptCheckingScheme, "full", "Check interrupts in every basic block");
RegisterComponent(InterruptCheckingScheme, BackwardsBranchCheckingScheme, "backwards", "Check interrupts in blocks that have backwards branches");

class TarjanInterruptScheme;
RegisterComponent(InterruptCheckingScheme, TarjanInterruptScheme, "tarjan", "Check interrupts using Tarjan's scheme");

InterruptCheckingScheme::~InterruptCheckingScheme()
{

}

bool NoneInterruptCheckingScheme::ApplyInterruptChecks(std::map<Address, TranslationBlockUnit *>& blocks)
{
	for (auto block : blocks) {
		block.second->SetInterruptCheck(false);
	}

	return true;
}

bool FullInterruptCheckingScheme::ApplyInterruptChecks(std::map<Address, TranslationBlockUnit *>& blocks)
{
	for (auto block : blocks) {
		block.second->SetInterruptCheck(true);
	}

	return true;
}

bool BackwardsBranchCheckingScheme::ApplyInterruptChecks(std::map<Address, TranslationBlockUnit *>& blocks)
{
	UNIMPLEMENTED;
	/*
	for (auto block : blocks) {
		if (block.second->HasSuccessors()) {
			for (auto succ : block.second->GetSuccessors()) {
				if (succ->GetOffset() <= block.first) {
					block.second->SetInterruptCheck(true);
					break;
				}
			}
		}

		bool direct, indirect;
		int32_t direct_offset, fallthrough_offset;
		block.second->GetCtrlFlowInfo(direct, indirect, direct_offset, fallthrough_offset);
		if(direct && direct_offset <= 0) block.second->SetInterruptCheck(true);

	}

	return true;
	*/
}

class TarjanInterruptScheme : public InterruptCheckingScheme
{
public:
	bool ApplyInterruptChecks(std::map<Address, TranslationBlockUnit *>& blocks) override
	{
		TarjanContext ctx;

		for (auto blockTBU : blocks) {
			TranslationBlockUnit *tbu = blockTBU.second;
			TarjanBlock block;

			// Initialise block metadata.
			block.tbu = tbu;
			block.block_addr = block.tbu->GetOffset();

			block.index = -1;
			block.lowlink = -1;

			block.is_valid = false;
			block.is_on_stack = false;

			// Ignore exit blocks and self loops.
			block.ignore = !block.tbu->HasSuccessors() || block.tbu->HasSelfLoop();

			// Mark blocks ignored at this stage as interrupt checks.
			if (block.ignore) {
				tbu->SetInterruptCheck(true);
			}

			ctx.tarjan_blocks[block.block_addr] = block;
		}

		return PerformStrongConnect(ctx);
	}

private:
	struct TarjanBlock {
		Address block_addr;
		uint32_t index;
		uint32_t lowlink;

		TranslationBlockUnit *tbu;

		bool ignore;
		bool is_valid;
		bool is_on_stack;
	};

	struct TarjanContext {
		uint32_t next_index, rmcount;

		std::map<Address, TarjanBlock> tarjan_blocks;
		std::list<Address> block_stack;
	};

	bool PerformStrongConnect(TarjanContext& ctx)
	{
		do {
			ctx.rmcount = 0;
			for (auto& block : ctx.tarjan_blocks) {
				// Skip ignored blocks.
				if (block.second.ignore)
					continue;

				// Skip blocks that are already interrupt blocks.
				if (block.second.tbu->IsInterruptCheck())
					continue;

				// Initialise the StrongConnect state, and run the algorithm.
				PrepareStrongConnect(ctx);
				StrongConnect(ctx, block.second);
			}
		} while(ctx.rmcount);

		return true;
	}

	void PrepareStrongConnect(TarjanContext& ctx)
	{
		ctx.next_index = 0;
		ctx.block_stack.clear();

		for (auto& block : ctx.tarjan_blocks) {
			block.second.is_valid = false;
			block.second.index = -1;
			block.second.lowlink = -1;
			block.second.is_on_stack = false;
		}
	}

	inline uint32_t min(uint32_t a, uint32_t b) const __attribute__((pure))
	{
		return ((a < b) ? a : b);
	}

	void StrongConnect(TarjanContext& ctx, TarjanBlock& block)
	{
		block.index = ctx.next_index;
		block.lowlink = ctx.next_index;
		block.is_valid = true;
		ctx.next_index++;

		ctx.block_stack.push_front(block.block_addr);
		block.is_on_stack = true;

		for (auto successorTBU : block.tbu->GetSuccessors()) {
			TarjanBlock& successor = ctx.tarjan_blocks[successorTBU->GetOffset()];

			// Ignore any edges to blocks marked as interrupt nodes
			if (successorTBU->IsInterruptCheck())
				continue;

			if (!successor.is_valid) {
				StrongConnect(ctx, successor);
				block.lowlink = min(block.lowlink, successor.lowlink);
			} else if (successor.is_on_stack) {
				block.lowlink = min(block.lowlink, successor.index);
			}
		}

		if (block.lowlink == block.index) {
			Address stacked_block;
			uint32_t count = 0;
			do {
				stacked_block = ctx.block_stack.front();
				ctx.block_stack.pop_front();
				ctx.tarjan_blocks[stacked_block].is_on_stack = false;

				count++;
			} while (stacked_block != block.block_addr);

			// Only consider SCCs which are made up of multiple nodes
			if (count > 1) {
				block.tbu->SetInterruptCheck(true);
				ctx.rmcount++;
			}
		}
	}
};
