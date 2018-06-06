#include "blockjit/block-compiler/transforms/Transform.h"

#include <vector>
#include <set>
#include <cstdint>
#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

DeadStoreElimination::~DeadStoreElimination() {
}

/*
 * Eliminate instructions which have no side effects and which write to 
 * vregs which are never read.
 */
bool DeadStoreElimination::Apply(TranslationContext& ctx)
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
