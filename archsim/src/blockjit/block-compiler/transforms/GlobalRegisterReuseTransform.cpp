/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * ConstantPropTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include <wutils/tick-timer.h>

#include <vector>
#include <set>
#include <cstdint>
#include <map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

GlobalRegisterReuseTransform::GlobalRegisterReuseTransform(const wutils::vbitset<>& used_registers, int max_regs) : used_pregs_(used_registers), max_regs_(max_regs)
{

}


GlobalRegisterReuseTransform::~GlobalRegisterReuseTransform()
{
}

bool GlobalRegisterReuseTransform::Apply(TranslationContext& ctx)
{
	int free_pregs = used_pregs_.inverted().count();
	int next_free_preg = used_pregs_.get_lowest_clear();

	// First, figure out which registers are frequently used
	std::map<uint32_t, uint32_t> register_frequencies;
	for(unsigned insn_idx = 0; insn_idx < ctx.count(); ++insn_idx) {
		auto insn = ctx.at(insn_idx);
		if(insn->type == IRInstruction::READ_REG) {
			if(insn->operands[0].is_constant()) {
				register_frequencies[insn->operands[0].value]++;
			}
		}
		if(insn->type == IRInstruction::WRITE_REG) {
			if(insn->operands[1].is_constant()) {
				register_frequencies[insn->operands[1].value]++;
			}
		}
	}

	// now,
	return true;
}
