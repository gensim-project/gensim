/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "blockjit/block-compiler/transforms/Transform.h"
#include "util/wutils/maybe-map.h"
#include "util/wutils/dense-set.h"
#include "util/wutils/small-set.h"

#include <vector>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

static void make_instruction_nop(IRInstruction *insn, bool set_block)
{
	insn->type = IRInstruction::NOP;
	insn->operands.clear();
	if(set_block) insn->ir_block = NOP_BLOCK;
}

RegisterAllocationTransform::RegisterAllocationTransform(uint32_t num_allocable_registers) : used_phys_regs_(num_allocable_registers), number_allocable_registers_(num_allocable_registers), stack_frame_size_(0) {
	
}

RegisterAllocationTransform::~RegisterAllocationTransform()
{

}

bool RegisterAllocationTransform::Apply(TranslationContext& ctx)
{
	IRBlockId latest_block_id = INVALID_BLOCK_ID;

	used_phys_regs_.clear();

	std::vector<int32_t> allocation (ctx.reg_count());
	maybe_map<IRRegId, uint32_t, 128> global_allocation (ctx.reg_count());	// global register allocation

	typedef dense_set<IRRegId> live_set_t;
	live_set_t live_ins(ctx.reg_count()), live_outs(ctx.reg_count());
	std::vector<live_set_t::iterator> to_erase;

	archsim::util::vbitset avail_regs (number_allocable_registers_); // Register indicies that are available for allocation.
	uint32_t next_global = 0;	// Next stack location for globally allocated register.

	std::vector<int32_t> vreg_seen_block (ctx.reg_count(), -1);

	// Build up a map of which vregs have been seen in which blocks, to detect spanning vregs.
	int32_t ir_idx;
	for (ir_idx = 0; ir_idx < (int32_t)ctx.count(); ir_idx++) {
		IRInstruction *insn = ctx.at(ir_idx);

		if(insn->ir_block == NOP_BLOCK) break;
		if(insn->type == IRInstruction::BARRIER) next_global = 0;

		for (int op_idx = 0; op_idx < insn->operands.size(); op_idx++) {
			IROperand *oper = &insn->operands[op_idx];
			if(!oper->is_valid()) break;

			// If the operand is a vreg, and is not already a global...
			if (oper->is_vreg() && (global_allocation.count(oper->value) == 0)) {
				auto seen_in_block = vreg_seen_block[oper->value];

				// If we have already seen this operand, and not in the same block, then we
				// must globally allocate it.
				if (seen_in_block != -1 && seen_in_block != (int32_t)insn->ir_block) {
					global_allocation[oper->value] = next_global;
					next_global += 8;
					if(next_global > stack_frame_size_) stack_frame_size_ = next_global;
				}

				vreg_seen_block[oper->value] = insn->ir_block;
			}
		}
	}

	// ir_idx is set up by the previous loop to point either to the end of the context, or to the last NOP
	for (; ir_idx >= 0; ir_idx--) {
		// Grab a pointer to the instruction we're looking at.
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;

		// Make sure it has a descriptor.
		assert(insn->type < captive::shared::num_descriptors);
		const struct insn_descriptor *descr = &insn_descriptors[insn->type];

		// Detect a change in block
		if (latest_block_id != insn->ir_block) {
			// Clear the live-in working set and current allocations.
			live_ins.clear();
			allocation.assign(ctx.reg_count(), -1);

			// Reset the available register bitfield
			avail_regs.set_all();
			
			// Update the latest block id.
			latest_block_id = insn->ir_block;
		}
//
//		if(insn->type == IRInstruction::BARRIER) {
//			next_global = 0;
//		}

		// Clear the live-out set, and make every current live-in a live-out.
		live_outs.copy(live_ins);

		// Loop over the VREG operands and update the live-in set accordingly.
		for (int o = 0; o < insn->operands.size(); o++) {
			if (!insn->operands[o].is_valid()) break;
			if (insn->operands[o].type != IROperand::VREG) continue;

			IROperand *oper = &insn->operands[o];
			IRRegId reg = (IRRegId)oper->value;

			if (descr->format[o] == 'I') {
				// <in>
				live_ins.insert(reg);
			} else if (descr->format[o] == 'O') {
				// <out>
				live_ins.erase(reg);
			} else if (descr->format[o] == 'B') {
				// <in/out>
				live_ins.insert(reg);
			}
		}

		//printf("  [%03d] %10s", ir_idx, descr->mnemonic);

		// Release LIVE-OUTs that are not LIVE-INs
		for (auto out : live_outs) {
			if(global_allocation.count(out)) continue;
			if (!live_ins.count(out)) {
				assert(allocation[out] != -1);

				// Make the released register available again.
				avail_regs.set(allocation[out], 1);
			}
		}

		// Allocate LIVE-INs
		for (auto in : live_ins) {
			// If the live-in is not already allocated, allocate it.
			if (allocation[in] == -1 && global_allocation.count(in) == 0) {
				int32_t next_reg = avail_regs.get_lowest_set();
				
				if (next_reg == -1) {
					global_allocation[in] = next_global;
					next_global += 8;
					if(stack_frame_size_ < next_global) stack_frame_size_ = next_global;

				} else {
					allocation[in] = next_reg;

					avail_regs.set(next_reg, 0);
					used_phys_regs_.set(next_reg, 1);
				}
			}
		}

		// If an instruction has no output or both operands which are live outs then the instruction is dead
		bool not_dead = false;
		bool can_be_dead = !descr->has_side_effects;

		// Loop over operands to update the allocation information on VREG operands.
		for (unsigned op_idx = 0; op_idx < insn->operands.size(); op_idx++) {
			IROperand *oper = &insn->operands[op_idx];
			if(!oper->is_valid()) break;

			if (oper->is_vreg()) {
				if (descr->format[op_idx] != 'I' && live_outs.count(oper->value)) not_dead = true;

				// If this vreg has been allocated to the stack, then fill in the stack entry location here
				//auto global_alloc = global_allocation.find(oper->value);
				if(global_allocation.count(oper->value)) {
					oper->allocate(IROperand::ALLOCATED_STACK, global_allocation[oper->value]);
					if(descr->format[op_idx] == 'O' || descr->format[op_idx] == 'B') not_dead = true;
				} else {

					// Otherwise, if the value has been locally allocated, fill in the local allocation
					//auto alloc_reg = allocation.find((IRRegId)oper->value);

					if (allocation[oper->value] != -1) {
						oper->allocate(IROperand::ALLOCATED_REG, allocation[oper->value]);
					}

				}
			}
		}

		// Remove allocations that are not LIVE-INs
		for (auto out : live_outs) {
			if(global_allocation.count(out)) continue;
			if (!live_ins.count(out)) {
				assert(allocation[out] != -1);

				allocation[out] = -1;
			}
		}

		// If this instruction is dead, remove any live ins which are not live outs
		// in order to propagate dead instruction elimination information
		if(!not_dead && can_be_dead) {
			make_instruction_nop(insn, true);

			to_erase.clear();
			to_erase.reserve(live_ins.size());
			for(auto i = live_ins.begin(); i != live_ins.end(); ++i) {
				const auto &in = *i;
				if(global_allocation.count(in)) continue;
				if(live_outs.count(in) == 0) {
					assert(allocation[in] != -1);

					avail_regs.set(allocation[in], 1);

					allocation[in] = -1;
					to_erase.push_back(i);
				}
			}
			for(auto e : to_erase)live_ins.erase(*e);
		}
	}

	//printf("block %08x\n", tb.block_addr);
//	dump_ir();
//	timer.tick("Allocation");
	//printf("lol ");
//	timer.dump("Analysis");
	return true;
}

uint32_t RegisterAllocationTransform::GetStackFrameSize() const
{
	return stack_frame_size_;
}

archsim::util::vbitset RegisterAllocationTransform::GetUsedPhysRegs() const
{
	return used_phys_regs_;
}
