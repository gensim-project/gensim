/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_translate.h"

#include "translate/llvm/LLVMWorkUnitTranslator.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "translate/llvm/LLVMTranslationContext.h"

#include "translate/profile/Region.h"

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_os_ostream.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <translate/llvm/LLVMBlockTranslator.h>

using namespace archsim::translate;
using namespace archsim::translate::translate_llvm;

static llvm::TargetMachine *GetNativeMachine()
{
	static std::mutex lock;
	lock.lock();

	llvm::TargetMachine *machine = nullptr;

	if(machine == nullptr) {
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		machine = llvm::EngineBuilder().selectTarget();
		machine->setOptLevel(llvm::CodeGenOpt::Aggressive);
		machine->setFastISel(false);
	}

	lock.unlock();
	return machine;
}

LLVMWorkUnitTranslator::LLVMWorkUnitTranslator(gensim::BaseLLVMTranslate *txlt, llvm::LLVMContext &ctx) : target_machine_(GetNativeMachine()), translate_(txlt), llvm_ctx_(ctx)
{

}


void EmitControlFlow_FallOffPage(llvm::IRBuilder<> &builder, LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, LLVMRegionTranslationContext &region)
{
	builder.CreateBr(region.GetExitBlock(LLVMRegionTranslationContext::EXIT_REASON_NEXTPAGE));
}

void EmitControlFlow_DirectNonPred(llvm::IRBuilder<> &builder, LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, translate_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
{
	llvm::BasicBlock *taken_block = region.GetDispatchBlock();

	// Even though we're talking about physical addresses here, this still works
	// when virtual memory is in effect since we're only checking that the jump
	// hasn't left the page (which depends only on the offset, not the page)
	if(ji.JumpTarget >= unit.GetRegion().GetPhysicalBaseAddress() && ji.JumpTarget < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		// jump target is on page
		if(region.GetBlockMap().count(ji.JumpTarget.PageOffset())) {
			taken_block = region.GetBlockMap().at(ji.JumpTarget.PageOffset());
		}
	}

	builder.CreateBr(taken_block);
}

void EmitControlFlow_DirectPred(llvm::IRBuilder<> &builder, translate_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, translate_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
{
	llvm::BasicBlock *taken_block = region.GetDispatchBlock();
	llvm::BasicBlock *nontaken_block = region.GetDispatchBlock();
	llvm::Value *pc = translate->EmitRegisterRead(builder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);

	// we need to account for the fact that the PC is virtual, not physical.
	// subtract the physical PC, then re-add the physical jump target to get the virtual jump target
	llvm::Value *taken_pc = builder.CreateSub(pc, llvm::ConstantInt::get(pc->getType(), (unit.GetRegion().GetPhysicalBaseAddress() + block.GetOffset() + ctrlflow->GetOffset()).Get()));
	taken_pc = builder.CreateAdd(pc, llvm::ConstantInt::get(pc->getType(), ji.JumpTarget.Get()));

	// If physical jump target falls on this physical page, then all is fine: we're jumping from one block to another (or falling through)
	if((ji.JumpTarget >= unit.GetRegion().GetPhysicalBaseAddress()) && (ji.JumpTarget < (unit.GetRegion().GetPhysicalBaseAddress() + 4096))) {
		// jump target is on page
		archsim::Address offset = ji.JumpTarget.PageOffset();
		if(region.GetBlockMap().count(offset)) {
			taken_block = region.GetBlockMap().at(offset);
		}
	}

	// Ditto for nontaken block
	auto fallthrough_addr = unit.GetRegion().GetPhysicalBaseAddress() + block.GetOffset() + ctrlflow->GetOffset() + ctrlflow->GetDecode().Instr_Length;
	if(fallthrough_addr >= unit.GetRegion().GetPhysicalBaseAddress() && fallthrough_addr < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		if(region.GetBlockMap().count(fallthrough_addr.PageOffset())) {
			nontaken_block = region.GetBlockMap().at(fallthrough_addr.PageOffset());
		}
	}


	auto branch_was_taken = builder.CreateICmpEQ(pc, taken_pc);
	builder.CreateCondBr(branch_was_taken, taken_block, nontaken_block);
}

void EmitControlFlow_Indirect(llvm::IRBuilder<> &builder, translate_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, llvm::Value *virt_page_base, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, translate_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
{
	// for now, go straight back to dispatch block. If we knew if the jump
	// landed on the same page as it started, we could be a bit smarter.

	// 1. check to see if the indirect jump lands on the same page
	llvm::Value *pc = translate->EmitRegisterRead(builder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);
	auto new_page_base = builder.CreateAnd(pc, ~archsim::Address::PageMask);
	auto same_page = builder.CreateICmpEQ(virt_page_base, new_page_base);

	llvm::BasicBlock *same_page_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", region.GetFunction());
	builder.CreateCondBr(same_page, same_page_block, region.GetDispatchBlock());

	builder.SetInsertPoint(same_page_block);
	auto pc_offset = builder.CreateAnd(pc, archsim::Address::PageMask);
	auto indirect_switch = builder.CreateSwitch(pc_offset, region.GetExitBlock(translate_llvm::LLVMRegionTranslationContext::EXIT_REASON_NOBLOCK));
	for(auto target : block.GetSuccessors()) {
		indirect_switch->addCase((llvm::ConstantInt*)llvm::ConstantInt::get(pc->getType(), target->GetOffset().Get()), region.GetBlockMap().at(target->GetOffset()));
	}
}


static void EmitControlFlow(llvm::IRBuilder<> &builder, translate_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, llvm::Value *virt_page_base, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, translate_llvm::LLVMRegionTranslationContext &region)
{
	// several options here:
	// 1. an instruction which isn't a jump but happens to be at the end of a block. It either falls through to a block which exists (jump there) or falls off the page.
	// 2. a direct non predicated jump: br directly to target block if it is on this page and has been translated
	// 3. a direct predicated jump: check the PC and either branch to the target block (if it's on this page and has been translated) or to the fallthrough block (if on this page and translated)
	// 4. an indirect jump: create a switch statement for profiled branch targets, and add the dispatch block to it.

	auto decode = &ctrlflow->GetDecode();
	auto insn_pc = unit.GetRegion().GetPhysicalBaseAddress() + block.GetOffset() + ctrlflow->GetOffset();
	auto next_pc = insn_pc + decode->Instr_Length;
	auto next_offset = block.GetOffset() + ctrlflow->GetOffset() + decode->Instr_Length;

	gensim::JumpInfo ji;
	auto jip = unit.GetThread()->GetArch().GetISA(decode->isa_mode).GetNewJumpInfo();
	jip->GetJumpInfo(decode, insn_pc, ji);

	if(!ji.IsJump) {
		// situation 1
		if(next_offset.Get() >= archsim::Address::PageSize || region.GetBlockMap().count(next_offset) == 0) {
			EmitControlFlow_FallOffPage(builder, ctx, translate, region);
		} else {
			builder.CreateBr(region.GetBlockMap().at(next_offset));
		}
	} else {
		if(!ji.IsIndirect) {
			if(!ji.IsConditional) {
				// situation 2
				EmitControlFlow_DirectNonPred(builder, ctx, translate, unit, block, ctrlflow, region, ji);
			} else {
				// situation 3
				EmitControlFlow_DirectPred(builder, ctx, translate, unit, block, ctrlflow, region, ji);
			}
		} else {
			// situation 4
			EmitControlFlow_Indirect(builder, ctx, translate, unit, virt_page_base, block, ctrlflow, region, ji);
		}
	}

	delete jip;
}

std::pair<llvm::Module *, llvm::Function *> LLVMWorkUnitTranslator::TranslateWorkUnit(TranslationWorkUnit &unit)
{
	// Create a new LLVM translation context.
	LLVMOptimiser opt;

	// Create a new llvm module to contain the translation
	llvm::Module *module = new llvm::Module("region_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()), llvm_ctx_);
	module->setDataLayout(target_machine_->createDataLayout());
	module->setTargetTriple(target_machine_->getTargetTriple().normalize());

	auto i8ptrty = llvm::Type::getInt8PtrTy(llvm_ctx_);

	llvm::FunctionType *fn_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_ctx_), {i8ptrty, i8ptrty}, false);

	std::string fn_name = "fn_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get());
	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction(fn_name, fn_type);

	auto entry_block = llvm::BasicBlock::Create(llvm_ctx_, "entry_block", fn);

	gensim::BaseLLVMTranslate *translate = translate_;

//	translate->InitialiseFeatures(unit.GetThread());
//	translate->InitialiseIsaMode(unit.GetThread());
//	translate->SetDecodeContext(unit.GetThread()->GetEmulationModel().GetNewDecodeContext(*unit.GetThread()));

	auto ji = unit.GetThread()->GetArch().GetISA(0).GetNewJumpInfo();

	translate_llvm::LLVMTranslationContext ctx(module->getContext(), fn, unit.GetThread());

	llvm::IRBuilder<> entry_builder (entry_block);
	auto virtual_page_base = translate->EmitRegisterRead(entry_builder, ctx, unit.GetThread()->GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);
	virtual_page_base = entry_builder.CreateAnd(virtual_page_base, ~archsim::Address::PageMask);

	translate_llvm::LLVMRegionTranslationContext region_ctx(ctx, fn, unit, entry_block, translate);

	LLVMBlockTranslator block_txlt(ctx, translate_,fn);

	for(auto block : unit.GetBlocks()) {
		llvm::BasicBlock *startblock = region_ctx.GetBlockMap().at(block.first);
		llvm::BasicBlock *end_block = block_txlt.TranslateBlock(unit.GetRegion().GetPhysicalBaseAddress() + block.first, *block.second, startblock);

		llvm::IRBuilder<> builder(end_block);

		// insert an interrupt check if necessary
		if(block.second->IsInterruptCheck()) {
			// load has message
			auto has_message = ctx.GetStateBlockPointer(builder, "MessageWaiting");
			has_message = builder.CreateLoad(has_message);
			has_message = builder.CreateICmpNE(has_message, llvm::ConstantInt::get(has_message->getType(), 0));

			llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);
			builder.CreateCondBr(has_message, region_ctx.GetExitBlock(LLVMRegionTranslationContext::EXIT_REASON_MESSAGE), continue_block);

			builder.SetInsertPoint(continue_block);
		}

		// did we have a direct or indirect jump? if direct, then assume the jump is conditional
		EmitControlFlow(builder, ctx, translate_, unit, virtual_page_base, *block.second, block.second->GetInstructions().back(), region_ctx);

	}

	ctx.Finalize();

	if(archsim::options::Debug) {
		std::stringstream filename_str;
		filename_str << "fn_" << std::hex << unit.GetRegion().GetPhysicalBaseAddress();
		std::ofstream str(filename_str.str().c_str());
		llvm::raw_os_ostream llvm_str (str);
		module->print(llvm_str, nullptr);
	}

	// optimise
	opt.Optimise(module, target_machine_->createDataLayout());

	if(archsim::options::Debug) {
		std::stringstream filename_str;
		filename_str << "fn_opt_" << std::hex << unit.GetRegion().GetPhysicalBaseAddress();
		std::ofstream str(filename_str.str().c_str());
		llvm::raw_os_ostream llvm_str (str);
		module->print(llvm_str, nullptr);
	}

	return {module, fn};
}
