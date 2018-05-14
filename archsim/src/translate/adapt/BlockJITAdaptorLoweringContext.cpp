/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "translate/adapt/BlockJITAdaptorLoweringContext.h"

using namespace archsim::translate::adapt;

bool BlockJITLoweringContext::Prepare(const TranslationContext& ctx) {
	// TODO: add lowerers
	AddLowerer(IRInstruction::ADD, new BlockJITADDLowering());
	AddLowerer(IRInstruction::BARRIER, new BlockJITBARRIERLowering());

	return true;
}

bool BlockJITLoweringContext::LowerBlock(const TranslationContext& ctx, captive::shared::IRBlockId block_id, uint32_t block_start) {
	std::string function_name = target_fun_->getName().str() + "_" + std::to_string(block_id);
	llvm::Function *block_function = (llvm::Function*)target_module_->getOrInsertFunction(function_name, GetIRBlockFunctionType(target_module_->getContext()));

	const IRInstruction *insn = ctx.at(block_start);
	while (insn < ctx.end() && (insn->ir_block == NOP_BLOCK || insn->ir_block == block_id)) {
		if (!LowerInstruction(ctx, insn)) {
			//				LC_ERROR(LogLower) << "Could not lower IRInstruction";
			return false;
		}
	}

	return true;
}

llvm::Value* BlockJITLoweringContext::GetValueFor(const IROperand& operand) {
	UNIMPLEMENTED;
}

void BlockJITLoweringContext::SetValueFor(const IROperand& operand, llvm::Value* value) {
	UNIMPLEMENTED;
}
