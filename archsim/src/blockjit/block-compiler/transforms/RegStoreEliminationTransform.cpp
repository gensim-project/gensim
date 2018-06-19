/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * RegStoreEliminationTransform.cpp
 *
 *  Created on: 13 Oct 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/transforms/Transform.h"

#include <wutils/tick-timer.h>

#include <map>
#include <vector>
#include <set>
#include <cstdint>

using namespace captive::arch::jit;
using namespace captive::shared;
using namespace captive::arch::jit::transforms;

RegStoreEliminationTransform::~RegStoreEliminationTransform()
{

}

class RegWriteInfo {
public:
	uint32_t Size, Offset;
	IRInstruction *Write;
	
	RegWriteInfo(uint32_t size, uint32_t offset, IRInstruction *write) : Size(size), Offset(offset), Write(write) {}
};

// Remove writes which alias the given offset and size from the provided map
static void ClearPreviousWrites(std::map<uint32_t, RegWriteInfo>& writes, uint32_t offset, uint32_t size) 
{
	uint32_t begin = offset;
	uint32_t end = offset + size;
	std::vector<uint32_t> offsets;
	for(auto i : writes) {
		if(i.first <= end || i.first + i.second.Size <= begin) {
			offsets.push_back(i.first);
		}
	}
	
	for(auto i : offsets) {
		writes.erase(i);
	}
}

// Look through writes, and if there is a write which aliases with offset and size,
// then replace that write with a nop instruction
static void NopPreviousWrites(std::map<uint32_t, RegWriteInfo>& writes, uint32_t offset, uint32_t size) 
{
	uint32_t begin = offset;
	uint32_t end = offset + size;
	std::vector<uint32_t> offsets;
	for(auto i : writes) {
		if(i.second.Offset == offset && i.second.Size == size) {
			offsets.push_back(i.first);
		}
	}
	
	for(auto i : offsets) {
		writes.at(i).Write->make_nop();
		writes.erase(i);
	}
}


bool RegStoreEliminationTransform::Apply(TranslationContext &ctx)
{
	std::map<uint32_t, RegWriteInfo> prev_writes;

	for(unsigned int ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		auto &descr = insn_descriptors[insn->type];

		switch(insn->type) {
			case IRInstruction::JMP:
			case IRInstruction::BRANCH:
			case IRInstruction::READ_MEM:
			case IRInstruction::WRITE_MEM:
			case IRInstruction::LDPC:
			case IRInstruction::INCPC:
			case IRInstruction::TAKE_EXCEPTION:
			case IRInstruction::CALL:
			case IRInstruction::DISPATCH:
			case IRInstruction::RET:
			case IRInstruction::READ_DEVICE:
			case IRInstruction::WRITE_DEVICE:
			case IRInstruction::ADC_WITH_FLAGS:
				prev_writes.clear();
				break;

			case IRInstruction::READ_REG: {
				IROperand &offset = insn->operands[0];
				if(!offset.is_constant()) {
					prev_writes.clear();
					break;
				}

				assert(offset.is_constant());

				ClearPreviousWrites(prev_writes, offset.value, insn->operands[1].size);
				
				break;
			}
			case IRInstruction::WRITE_REG: {
				IROperand &offset = insn->operands[1];
				uint32_t size = insn->operands[0].size;
				if(!offset.is_constant()) {
					prev_writes.clear();
					break;
				}

				assert(offset.is_constant());

				// Only nop out an instruction if the prev write is smaller than
				// the new one
				NopPreviousWrites(prev_writes, offset.value, size);
				
				prev_writes.insert({offset.value, RegWriteInfo(offset.value, size, insn)});

				break;
			}
			default:
				break;
		}
	}
	return true;
}

