/*
 * RegValueReuseTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include "util/wutils/tick-timer.h"

#include <vector>
#include <set>
#include <cstdint>
#include <unordered_map>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

RegValueReuseTransform::~RegValueReuseTransform()
{

}

bool RegValueReuseTransform::Apply(TranslationContext &ctx)
{
	// Reuse values read from registers (rather than reloading from memory)
	// A value becomes live when it is
	// - read from a register into a vreg
	// - written from a vreg to a register
	// A value is killed when the vreg is written to (the value is overwritten)
	//

	// Maps of register offsets to vregs
	std::unordered_map<uint32_t, IROperand> offset_to_vreg;
	std::unordered_map<IROperand, uint32_t> vreg_to_offset;

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		auto &descr = insn_descriptors[insn->type];

		// First, check to see if the vregs containing any live values have been killed
		for(unsigned int op_idx = 0; op_idx < 6; ++op_idx) {
			const IROperand &op = insn->operands[op_idx];

			// If this operand is written to
			if(descr.format[op_idx] == 'O' || descr.format[op_idx] == 'B') {
				if(vreg_to_offset.count(op)) {
					offset_to_vreg.erase(vreg_to_offset.at(op));
					vreg_to_offset.erase(op);
				}
			}
		}

		switch(insn->type) {
			case IRInstruction::JMP:
			case IRInstruction::BRANCH:
			case IRInstruction::READ_MEM:
			case IRInstruction::READ_MEM_USER:
			case IRInstruction::WRITE_MEM:
			case IRInstruction::WRITE_MEM_USER:
			case IRInstruction::LDPC:
			case IRInstruction::INCPC:
			case IRInstruction::TAKE_EXCEPTION:
			case IRInstruction::CALL:
			case IRInstruction::DISPATCH:
			case IRInstruction::RET:
			case IRInstruction::READ_DEVICE:
			case IRInstruction::WRITE_DEVICE:
			case IRInstruction::ADC_WITH_FLAGS:

				offset_to_vreg.clear();
				vreg_to_offset.clear();
				break;


			case IRInstruction::READ_REG: {
				// If this is a register read, make a value live
				IROperand &offset = insn->operands[0];
				IROperand &vreg = insn->operands[1];

				if(!offset.is_constant()) {
					// if we're reading an unknown register, clear everything
					offset_to_vreg.clear();
					vreg_to_offset.clear();
					break;
				}

				assert(vreg.is_vreg());
				assert(offset.is_constant());

				// If we have the correct register in a vreg and it  has exactly
				// the right size, then reuse the loaded value.
				if(offset_to_vreg.count(offset.value) && offset_to_vreg.at(offset.value).size == vreg.size) {
					// reuse the already-live value for this instruction
					insn->type = IRInstruction::MOV;
					insn->operands[0] = offset_to_vreg[offset.value];
				} else {

					for(auto i : offset_to_vreg) {
						bool i_fully_before_vreg = (i.first + i.second.size) <= offset.value;
						bool i_fully_after_vreg = i.first >= (offset.value + vreg.size);

						if(!(i_fully_before_vreg || i_fully_after_vreg)) {
							// we have an alias so remove i from the live vales.
							vreg_to_offset.erase(i.second);
							offset_to_vreg.erase(i.first);
							break;
						}
					}

					// mark the value as live
					offset_to_vreg[offset.value] = vreg;
					vreg_to_offset[vreg] = offset.value;
				}

				break;
			}
			case IRInstruction::WRITE_REG: {
				// if this is a write to any live regs, kill them
				IROperand &offset = insn->operands[1];
				IROperand &value = insn->operands[0];

				if(!offset.is_constant()) {
					// if we're writing an unknown register, clear everything
					offset_to_vreg.clear();
					vreg_to_offset.clear();
					break;
				}

				// we are accessing the register bank at OFFSET. we have a
				// problem if there is a live value with a key which is
				// less than OFFSET, but a key+size which is greater than
				// offset (the case where the key == OFFSET is already
				// covered).

				// for now do this naively by looping through the loaded
				// values. There is certainly a more efficient way to do
				// this using an ordered map.
				for(auto i : offset_to_vreg) {
					bool i_fully_before_vreg = (i.first + i.second.size) <= offset.value;
					bool i_fully_after_vreg = i.first >= (offset.value + value.size);

					if(!(i_fully_before_vreg || i_fully_after_vreg)) {
						// we have an alias so remove i from the live vales.
						vreg_to_offset.erase(i.second);
						offset_to_vreg.erase(i.first);
						break;
					}
				}

				offset_to_vreg[offset.value] = value;
				if(value.is_vreg()) vreg_to_offset[value] = offset.value;

				break;
			}
			default:
				break;
		}
	}

	return true;
}
