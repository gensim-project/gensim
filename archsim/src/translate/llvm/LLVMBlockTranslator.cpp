/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/llvm/LLVMBlockTranslator.h"
#include "translate/llvm/LLVMTranslationContext.h"

#include <llvm/IR/BasicBlock.h>

using namespace archsim::translate::translate_llvm;

LLVMBlockTranslator::LLVMBlockTranslator(LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *txlt, llvm::Function *target_fn) : txlt_(txlt), target_fn_(target_fn), ctx_(ctx)
{

}

void TranslateInstructionPredicated(llvm::IRBuilder<> &builder, LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address insn_pc, llvm::Function *fn)
{
	if(archsim::options::Trace) {
		builder.CreateCall(ctx.Functions.cpuTraceInstruction, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get()), llvm::ConstantInt::get(ctx.Types.i32, decode->ir), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 1)});
	}

	gensim::JumpInfo ji;
	auto jip = thread->GetArch().GetISA(decode->isa_mode).GetNewJumpInfo();
	jip->GetJumpInfo(decode, insn_pc, ji);

	llvm::Value *predicate_passed = translate->EmitPredicateCheck(builder, ctx, thread, decode, insn_pc, fn);
	predicate_passed = builder.CreateICmpNE(predicate_passed, llvm::ConstantInt::get(predicate_passed->getType(), 0));

	llvm::BasicBlock *passed_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);
	llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);
	llvm::BasicBlock *failed_block = continue_block;

	if(ji.IsJump) {
		// if we're translating a predicated jump, we need to increment the PC only if the predicate fails
		failed_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);
		llvm::IRBuilder<> failed_builder(failed_block);

		ctx.StoreGuestRegister(failed_builder, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr, llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get() + decode->Instr_Length));
		failed_builder.CreateBr(continue_block);

	}

	builder.CreateCondBr(predicate_passed, passed_block, failed_block);

	builder.SetInsertPoint(passed_block);
	translate->TranslateInstruction(builder, ctx, thread, decode, insn_pc, fn);
	builder.CreateBr(continue_block);

	builder.SetInsertPoint(continue_block);
	// update PC


	if(archsim::options::Trace) {
		builder.CreateCall(ctx.Functions.cpuTraceInsnEnd, {ctx.GetThreadPtr(builder)});
	}

	if(!ji.IsJump) {
		ctx.StoreGuestRegister(builder, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr, llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get() + decode->Instr_Length));
	}

	delete jip;
}


llvm::BasicBlock *LLVMBlockTranslator::TranslateBlock(Address block_base, TranslationBlockUnit &block, llvm::BasicBlock *target_block)
{
	llvm::IRBuilder<> builder(target_block);

	if(archsim::options::Verbose) {
		// increment instruction counter
		txlt_->EmitIncrementCounter(builder, ctx_, ctx_.GetThread()->GetMetrics().InstructionCount, block.GetInstructions().size());
		txlt_->EmitIncrementCounter(builder, ctx_, ctx_.GetThread()->GetMetrics().JITInstructionCount, block.GetInstructions().size());
	}

	auto ji = ctx_.GetThread()->GetArch().GetISA(0).GetNewJumpInfo();

	for(auto insn : block.GetInstructions()) {
		auto insn_pc = block_base + insn->GetOffset();

		if(insn->GetDecode().GetIsPredicated()) {
			TranslateInstructionPredicated(builder, ctx_, txlt_, ctx_.GetThread(), &insn->GetDecode(), insn_pc, target_fn_);
		} else {
			if(archsim::options::Trace) {
				builder.CreateCall(ctx_.Functions.cpuTraceInstruction, {ctx_.GetThreadPtr(builder), llvm::ConstantInt::get(ctx_.Types.i64, insn_pc.Get()), llvm::ConstantInt::get(ctx_.Types.i32, insn->GetDecode().ir), llvm::ConstantInt::get(ctx_.Types.i32, 0), llvm::ConstantInt::get(ctx_.Types.i32, 0), llvm::ConstantInt::get(ctx_.Types.i32, 1)});
			}

			txlt_->TranslateInstruction(builder, ctx_, ctx_.GetThread(), &insn->GetDecode(), insn_pc, target_fn_);

			if(archsim::options::Trace) {
				builder.CreateCall(ctx_.Functions.cpuTraceInsnEnd, {ctx_.GetThreadPtr(builder)});
			}

			gensim::JumpInfo jumpinfo;
			ji->GetJumpInfo(&insn->GetDecode(), Address(0), jumpinfo);
			if(!jumpinfo.IsJump) {
				auto &descriptor = ctx_.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC");
				llvm::Type *type;
				switch(descriptor.GetEntrySize()) {
					case 4:
						type = ctx_.Types.i32;
						break;
					case 8:
						type = ctx_.Types.i64;
						break;
					default:
						UNEXPECTED;
				}

				ctx_.StoreGuestRegister(builder, descriptor, nullptr, llvm::ConstantInt::get(type, (insn_pc + insn->GetDecode().Instr_Length).Get()));
			}

		}
		ctx_.ResetRegisters();

	}

	delete ji;
	return builder.GetInsertBlock();
}
