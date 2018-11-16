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

	if((ji.JumpTarget >= unit.GetRegion().GetPhysicalBaseAddress()) && (ji.JumpTarget < (unit.GetRegion().GetPhysicalBaseAddress() + 4096))) {
		// jump target is on page
		archsim::Address offset = ji.JumpTarget.PageOffset();
		if(region.GetBlockMap().count(offset)) {
			taken_block = region.GetBlockMap().at(offset);
		}
	}

	auto fallthrough_addr = unit.GetRegion().GetPhysicalBaseAddress() + block.GetOffset() + ctrlflow->GetOffset() + ctrlflow->GetDecode().Instr_Length;
	if(fallthrough_addr >= unit.GetRegion().GetPhysicalBaseAddress() && fallthrough_addr < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		if(region.GetBlockMap().count(fallthrough_addr.PageOffset())) {
			nontaken_block = region.GetBlockMap().at(fallthrough_addr.PageOffset());
		}
	}

	llvm::Value *pc = translate->EmitRegisterRead(builder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);
	auto branch_was_taken = builder.CreateICmpEQ(pc, llvm::ConstantInt::get(ctx.Types.i64, ji.JumpTarget.Get()));
	builder.CreateCondBr(branch_was_taken, taken_block, nontaken_block);
}

void EmitControlFlow_Indirect(llvm::IRBuilder<> &builder, translate_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, translate_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
{
	llvm::Value *pc_val = translate->EmitRegisterRead(builder, ctx, ctx.GetArch().GetRegisterFileDescriptor().GetTaggedEntry("PC"), nullptr);
	llvm::SwitchInst *out_switch = builder.CreateSwitch(pc_val, region.GetDispatchBlock());
	for(auto &succ : block.GetSuccessors()) {
		auto succ_addr = (unit.GetRegion().GetPhysicalBaseAddress() + succ->GetOffset());

		if(region.GetBlockMap().count(succ->GetOffset())) {
			out_switch->addCase(llvm::ConstantInt::get(ctx.Types.i64, succ_addr.Get()), region.GetBlockMap().at(succ->GetOffset()));
		}
	}
}


void EmitControlFlow(llvm::IRBuilder<> &builder, translate_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, translate_llvm::LLVMRegionTranslationContext &region)
{
	// several options here:
	// 1. an instruction which isn't a jump but happens to be at the end of a block (due to being at the end of the page): return since we can't do anything
	// 2. a direct non predicated jump: br directly to target block if it is on this page and has been translated
	// 3. a direct predicated jump: check the PC and either branch to the target block (if it's on this page and has been translated) or to the fallthrough block (if on this page and translated)
	// 4. an indirect jump: create a switch statement for profiled branch targets, and add the dispatch block to it.

	auto decode = &ctrlflow->GetDecode();
	auto insn_pc = unit.GetRegion().GetPhysicalBaseAddress() + block.GetOffset() + ctrlflow->GetOffset();

	gensim::JumpInfo ji;
	auto jip = unit.GetThread()->GetArch().GetISA(decode->isa_mode).GetNewJumpInfo();
	jip->GetJumpInfo(decode, insn_pc, ji);

	if(!ji.IsJump) {
		// situation 1
		EmitControlFlow_FallOffPage(builder, ctx, translate, region);
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
			EmitControlFlow_Indirect(builder, ctx, translate, unit, block, ctrlflow, region, ji);
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

	translate_llvm::LLVMRegionTranslationContext region_ctx(ctx, fn, unit, entry_block, translate);

	LLVMBlockTranslator block_txlt(ctx, translate_,fn);

	for(auto block : unit.GetBlocks()) {
		llvm::BasicBlock *startblock = region_ctx.GetBlockMap().at(block.first);
		llvm::BasicBlock *end_block = block_txlt.TranslateBlock(unit.GetRegion().GetPhysicalBaseAddress() + block.first, *block.second, startblock);

		// did we have a direct or indirect jump? if direct, then assume the jump is conditional
		llvm::IRBuilder<> builder(end_block);
		EmitControlFlow(builder, ctx, translate_, unit, *block.second, block.second->GetInstructions().back(), region_ctx);

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
