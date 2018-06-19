/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "blockjit/block-compiler/transforms/Transform.h"
#include "util/wutils/maybe-map.h"
#include "util/wutils/dense-set.h"
#include "util/wutils/small-set.h"

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

GlobalRegisterAllocationTransform::GlobalRegisterAllocationTransform(uint32_t num_allocable_registers) : num_allocable_registers_(num_allocable_registers), used_phys_regs_(num_allocable_registers)
{
	used_phys_regs_.clear();
}


GlobalRegisterAllocationTransform::~GlobalRegisterAllocationTransform() {
}

/*
 *
 * Allocate registers in 4 steps:
 *   First, identify live ranges. The input code is topologically sorted (i.e.,
 * there are no backwards branches), so this is as straightforward as identifying
 * the first and last uses of each vreg.
 *   Secondly, allocate 'virtual' registers to each vreg. This is essentially
 * just counting how many physical registers would be needed to allocate
 * every register
 *   Thirdly, allocate these virtual registers either to physical registers
 * or to the stack if there are none left. This is still not super smart since
 * it means that 'deeply' allocated registers will always go to the stack,
 * ignoring e.g. how frequently they are used. However it is good enough for 
 * now.
 *   Finally, go back through the IR and attach the allocations on to each
 * allocated vreg. 
 * 
 */
bool GlobalRegisterAllocationTransform::Apply(TranslationContext& ctx)
{
	std::vector<unsigned> vreg_begins(ctx.reg_count(), 0xffffffff), vreg_ends(ctx.reg_count(), 0);
	
	for(unsigned insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto insn = ctx.at(insn_idx);
		
		for(unsigned op_idx = 0; op_idx < insn->operands.size(); op_idx++) {
			auto &operand = insn->operands.at(op_idx);
			if(operand.is_vreg()) {
				if(vreg_begins[operand.get_vreg_idx()] > insn_idx) {
					vreg_begins[operand.get_vreg_idx()] = insn_idx;
				}
				if(vreg_ends[operand.get_vreg_idx()] < insn_idx) {
					vreg_ends[operand.get_vreg_idx()] = insn_idx;
				}
			}
		}
	}
	
	// first, reallocate vregs
	std::vector<unsigned> allocations(ctx.reg_count(), 0xffffffff);
	std::vector<unsigned> free_regs;
	unsigned next_free_reg = 0;
	unsigned total_regs = 0;
	
	for(unsigned insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto insn = ctx.at(insn_idx);
		for(unsigned op_idx = 0; op_idx < insn->operands.size(); op_idx++) {
			auto &operand = insn->operands.at(op_idx);
			if(operand.is_vreg()) {
				if(insn_idx == vreg_begins[operand.get_vreg_idx()]) {
					// allocate a reg for vreg
					int reg_id;
					
					if(free_regs.size()) {
						reg_id = free_regs.back();
						free_regs.pop_back();
					} else {
						reg_id = next_free_reg++;
						total_regs++;
					}
					
					allocations[operand.get_vreg_idx()] = reg_id;
				}
				if(insn_idx == vreg_ends[operand.get_vreg_idx()]) {
					free_regs.push_back(allocations[operand.get_vreg_idx()]);
				}
			}
		}
	}
	
	uint32_t regs_left = num_allocable_registers_;
	uint32_t next_stack_frame = 0;
	
	std::vector<std::pair<IROperand::IRAllocationMode, int>> allocation_info(total_regs, std::make_pair(IROperand::NOT_ALLOCATED, 0));
	for(unsigned reg_idx = 0; reg_idx < total_regs; ++reg_idx) {
		auto &allocation = allocation_info[reg_idx];
		
		if(regs_left) {
			allocation = std::make_pair(IROperand::ALLOCATED_REG, num_allocable_registers_-regs_left);
			used_phys_regs_.set(num_allocable_registers_-regs_left, 1);
			regs_left--;
		} else {
			allocation = std::make_pair(IROperand::ALLOCATED_STACK, next_stack_frame);
			next_stack_frame += 8;
		}
	}
	
	for(unsigned insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto insn = ctx.at(insn_idx);
		for(unsigned op_idx = 0; op_idx < insn->operands.size(); op_idx++) {
			auto &operand = insn->operands.at(op_idx);
			if(operand.is_vreg()) {
				auto allocation = allocation_info[allocations[operand.get_vreg_idx()]];
				operand.allocate(allocation.first, allocation.second);
			}
		}
	}
	
	stack_frame_size_ = next_stack_frame;
	
	return true;
}

uint32_t GlobalRegisterAllocationTransform::GetStackFrameSize() const
{
	return stack_frame_size_;
}

archsim::util::vbitset GlobalRegisterAllocationTransform::GetUsedPhysRegs() const
{
	return used_phys_regs_;
}
