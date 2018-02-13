/*
 * LoweringContext.cpp
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#include "blockjit/block-compiler/lowering/InstructionLowerer.h"
#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "blockjit/block-compiler/lowering/Finalisation.h"
#include "util/LogContext.h"

UseLogContext(LogBlockJit);
DeclareChildLogContext(LogLower, LogBlockJit, "Lowering");

using namespace captive::shared;
using namespace captive::arch::jit;
using namespace captive::arch::jit::lowering;

LoweringContext::LoweringContext(uint32_t stack_size) : _stack_frame_size(stack_size)
{
	_lowerers.resize(IRInstruction::_END, NULL);
}

LoweringContext::~LoweringContext()
{

}

bool LoweringContext::PrepareLowerers(const TranslationContext &tctx, BlockCompiler &compiler)
{
	for(auto i : _lowerers) if(i) i->SetContexts(*this, tctx, compiler);
	return true;
}

bool LoweringContext::Lower(const TranslationContext &ctx)
{
	if(!LowerHeader(ctx)) {
		LC_ERROR(LogLower) << "Failed to lower function header";
		return false;
	}
	if(!LowerBody(ctx)) {
		LC_ERROR(LogLower) << "Failed to lower function body";
		return false;
	}

	if(!PerformFinalisations()) {
		LC_ERROR(LogLower) << "Failed to perform finalisations";
		return false;
	}

	if(!PerformRelocations(ctx)) {
		LC_ERROR(LogLower) << "Failed to perform relocations";
		return false;
	}

	return true;
}

bool LoweringContext::RegisterBlockOffset(captive::shared::IRBlockId block, offset_t offset)
{
	if(_block_offsets.size() <= block) {
		_block_offsets.resize(block+1);
	}

	_block_offsets[block] = offset;
	return true;
}

bool LoweringContext::RegisterBlockRelocation(offset_t offset, shared::IRBlockId block)
{
	_block_relocations.push_back({offset, block});
	return true;
}

bool LoweringContext::RegisterFinalisation(Finalisation *f)
{
	_finalisations.push_back(f);
	return true;
}

bool LoweringContext::PerformFinalisations()
{
	for(auto i : _finalisations) {
		i->Finalise(*this);
		delete i;
	}
	_finalisations.clear();
	return true;
}

bool LoweringContext::LowerBody(const TranslationContext &ctx)
{
	LC_DEBUG4(LogLower) << "Lowering body";
	uint32_t current_block = NOP_BLOCK;
	for(uint32_t ir_idx = 0; ir_idx < ctx.count(); ++ir_idx) {
		IRInstruction *insn = ctx.at(ir_idx);
		if(insn->ir_block == NOP_BLOCK) continue;
		if(insn->ir_block != current_block) {
			current_block = insn->ir_block;
			if(!LowerBlock(ctx, current_block, ir_idx)) {
				LC_ERROR(LogLower) << "Could not lower IR block";
				return false;
			}
		}
	}

	return true;
}

bool LoweringContext::LowerBlock(const TranslationContext &ctx, captive::shared::IRBlockId block_id, uint32_t block_start)
{
	LC_DEBUG4(LogLower) << "Lowering block";
	RegisterBlockOffset(block_id, GetEncoderOffset());

	const IRInstruction *insn = ctx.at(block_start);
	while(insn < ctx.end() && (insn->ir_block == NOP_BLOCK || insn->ir_block == block_id)) {
		if(!LowerInstruction(ctx, insn)) {
			LC_ERROR(LogLower) << "Could not lower IRInstruction";
			return false;
		}
	}

	return true;
}

bool LoweringContext::LowerInstruction(const TranslationContext &ctx, const captive::shared::IRInstruction *&insn)
{
	LC_DEBUG4(LogLower) << "Lowering instruction";
	if(_lowerers[insn->type]) {
		// Make sure we advance the instruction pointer
		auto *pre_ptr = insn;
		bool success = _lowerers[insn->type]->Lower(insn);

		if(!success) {
			LC_ERROR(LogLower) << "Failed to lower instruction of type " << insn->type;
		} else {
			assert(insn > pre_ptr);
		}
		return success;
	}

	LC_ERROR(LogLower) << "Unknown instruction type " << captive::shared::insn_descriptors[insn->type].mnemonic;

	return false;
}

bool LoweringContext::AddLowerer(captive::shared::IRInstruction::IRInstructionType type, InstructionLowerer* lowerer)
{
	if(_lowerers.at(type)) {
		LC_ERROR(LogLower) << "Lowerer for instruction type " << type << " already registered";
		return false;
	}
	_lowerers[type] = lowerer;
	return true;
}
