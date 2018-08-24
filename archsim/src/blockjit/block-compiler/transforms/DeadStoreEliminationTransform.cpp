/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "blockjit/block-compiler/transforms/Transform.h"

#include <vector>
#include <set>
#include <cstdint>
#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

DeadStoreElimination::~DeadStoreElimination()
{
}

/*
 * Eliminate instructions which have no side effects and which write to
 * vregs which are never read.
 *
 * This relies on the fact that the IR is topologically sorted.
 */
bool DSE(TranslationContext &ctx)
{

	std::vector<bool> vreg_read (ctx.reg_count(), false);

	for(int insn_idx = ctx.count()-1; insn_idx >= 0; insn_idx--) {
		auto insn = ctx.at(insn_idx);
		auto &descriptor = insn->descriptor();
		bool should_eliminate = false;

		for(int op_idx = insn->operands.size()-1; op_idx >= 0; --op_idx) {
			auto &op = insn->operands.at(op_idx);

			if(op.is_vreg()) {

				if(!descriptor.has_side_effects) {
					if(descriptor.format[op_idx] == 'O' || descriptor.format[op_idx] == 'B') {
						if(!vreg_read[op.get_vreg_idx()]) {
							should_eliminate = true;
							break;
						}
					}
				}
				if(descriptor.format[op_idx] == 'I' || descriptor.format[op_idx] == 'B') {
					vreg_read[op.get_vreg_idx()] = true;
				}
			}
		}

		if(should_eliminate) {
			insn->make_nop();
		}
	}

	return true;
}

bool StoreAfterStoreElimination(TranslationContext &ctx)
{

	std::map<IRRegId, IRInstruction*> prev_writes;

	for(unsigned int insn_idx = 0; insn_idx < ctx.count(); insn_idx++) {
		auto insn = ctx.at(insn_idx);
		auto &descriptor = insn->descriptor();

		for(unsigned int op_idx = 0; op_idx < insn->operands.size(); ++op_idx) {
			auto &op = insn->operands.at(op_idx);

			if(op.is_vreg()) {
				if(descriptor.format[op_idx] == 'I' || descriptor.format[op_idx] == 'B') {
					prev_writes.erase(op.get_vreg_idx());
				}
				if(descriptor.format[op_idx] == 'O' || descriptor.format[op_idx] == 'B') {
					if(prev_writes.count(op.get_vreg_idx())) {
						if(insn->ir_block == prev_writes[op.get_vreg_idx()]->ir_block) {
							prev_writes[op.get_vreg_idx()]->make_nop();
						}
					}
					prev_writes[op.get_vreg_idx()] = insn;
				}

			}
		}
	}

	return true;
}

bool DeadStoreElimination::Apply(TranslationContext& ctx)
{
	return DSE(ctx) && StoreAfterStoreElimination(ctx);
}
