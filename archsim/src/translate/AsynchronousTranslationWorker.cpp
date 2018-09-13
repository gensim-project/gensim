/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "translate/AsynchronousTranslationWorker.h"
#include "translate/AsynchronousTranslationManager.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMTranslation.h"
#include "translate/adapt/BlockJITToLLVM.h"
#include "blockjit/BlockJitTranslate.h"
#include "gensim/gensim_processor_api.h"

#include "util/LogContext.h"
#include "translate/profile/Block.h"

#include "translate/Translation.h"
#include "translate/jit_funs.h"
#include "translate/llvm/LLVMMemoryManager.h"

#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_os_ostream.h>

#include <iostream>
#include <iomanip>
#include <set>

UseLogContext(LogTranslate);
UseLogContext(LogWorkQueue);

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
		machine->setOptLevel(llvm::CodeGenOpt::Default);
		machine->setFastISel(true);
	}

	lock.unlock();
	return machine;
}

AsynchronousTranslationWorker::AsynchronousTranslationWorker(AsynchronousTranslationManager& mgr, uint8_t id, gensim::BaseLLVMTranslate *translate) :
	Thread("Txln Worker"),
	mgr(mgr),
	terminate(false),
	id(id),
	optimiser(NULL),
	translate_(translate),
	target_machine_(GetNativeMachine()),
	memory_manager_(new archsim::translate::translate_llvm::LLVMMemoryManager(code_pool, code_pool)),
	linker_([&]()
{
	return memory_manager_;
}),
compiler_(linker_, llvm::orc::SimpleCompiler(*target_machine_))
{

}

/**
 * The main thread entry-point for this translation worker.
 */
void AsynchronousTranslationWorker::run()
{

	// Instantiate the unique lock for the work unit queue.
	std::unique_lock<std::mutex> queue_lock(mgr.work_unit_queue_lock, std::defer_lock);

	// Loop until told to terminate.
	while (!terminate) {
		// Acquire the work unit queue lock.
		queue_lock.lock();

		// Loop until work becomes available, or we're asked to terminate.
		while (mgr.work_unit_queue.empty() && !terminate) {
			// first do a bit of busy work
			code_pool.GC();

			// Wait on the work unit queue signal.
			mgr.work_unit_queue_cond.wait(queue_lock);
		}

		// If we woke up because we were asked to terminate, then release the
		// work unit queue lock and exit the loop.
		if (terminate) {
			queue_lock.unlock();
			break;
		}

		// Dequeue the translation work unit.
		TranslationWorkUnit *unit;

		// Skip over work units that are invalid.
		do {
			// Detect the case when we've cleared the work unit queue of valid translations.
			if (mgr.work_unit_queue.empty()) {
				unit = NULL;
				break;
			}

			unit = mgr.work_unit_queue.top();
			mgr.work_unit_queue.pop();

			if (!unit->GetRegion().IsValid()) {
				LC_DEBUG1(LogWorkQueue) << "[DEQUEUE] Skipping " << *unit;
				delete unit;

				//Setting unit to null here will cause us to break out of this do, while loop,
				//and then hit the continue a little later on in the main while loop. This
				//entire loop is somewhat too complicated and should be broken up.
				unit = NULL;
				break;
			}
		} while (!terminate && !unit->GetRegion().IsValid());

		// Release the queue lock.
		queue_lock.unlock();

		if (terminate || !unit)
			continue;

		LC_DEBUG1(LogWorkQueue) << "[DEQUEUE] Dequeueing " << *unit << ", queue length " << mgr.work_unit_queue.size() << ", @ " << (uint32_t)id;

		// Perform the translation, and destroy the translation work unit.
		::llvm::LLVMContext llvm_ctx;
		Translate(llvm_ctx, *unit);
		delete unit;
	}

}

/**
 * Stops the translation worker thread.
 */
void AsynchronousTranslationWorker::stop()
{
	// Set the termination flag.
	terminate = true;

	// Signal the thread to wake-up, if it's waiting on the work unit queue lock.
	mgr.work_unit_queue_lock.lock();
	mgr.work_unit_queue_cond.notify_all();
	mgr.work_unit_queue_lock.unlock();

	// Wait for the thread to terminate.
	join();
}

static unsigned __int128 uremi128(unsigned __int128 a, unsigned __int128 b)
{
	return a % b;
}
static unsigned __int128 udivi128(unsigned __int128 a, unsigned __int128 b)
{
	return a / b;
}
static __int128 remi128(__int128 a, __int128 b)
{
	return a % b;
}
static __int128 divi128(__int128 a, __int128 b)
{
	return a / b;
}

static bool shunt_builtin_f32_is_snan(archsim::core::thread::ThreadInstance* thread, float f)
{
	return thread->fn___builtin_f32_is_snan(f);
}
static bool shunt_builtin_f32_is_qnan(archsim::core::thread::ThreadInstance* thread, float f)
{
	return thread->fn___builtin_f32_is_qnan(f);
}
static bool shunt_builtin_f64_is_snan(archsim::core::thread::ThreadInstance* thread, double f)
{
	return thread->fn___builtin_f64_is_snan(f);
}
static bool shunt_builtin_f64_is_qnan(archsim::core::thread::ThreadInstance* thread, double f)
{
	return thread->fn___builtin_f64_is_qnan(f);
}
LLVMTranslation* AsynchronousTranslationWorker::CompileModule(TranslationWorkUnit& unit, ::llvm::Module* module, llvm::Function* function)
{
	std::map<std::string, void *> jit_symbols;

	jit_symbols["cpuTrap"] = (void*)cpuTrap;
	jit_symbols["cpuTakeException"] = (void*)cpuTakeException;
	jit_symbols["cpuReadDevice"] = (void*)devReadDevice;
	jit_symbols["cpuWriteDevice"] = (void*)devWriteDevice;
	jit_symbols["cpuSetFeature"] = (void*)cpuSetFeature;

	jit_symbols["cpuSetRoundingMode"] = (void*)cpuSetRoundingMode;
	jit_symbols["cpuGetRoundingMode"] = (void*)cpuGetRoundingMode;
	jit_symbols["cpuSetFlushMode"] = (void*)cpuSetFlushMode;
	jit_symbols["cpuGetFlushMode"] = (void*)cpuGetFlushMode;

	jit_symbols["genc_adc_flags"] = (void*)genc_adc_flags;
	jit_symbols["blkRead8"] = (void*)blkRead8;
	jit_symbols["blkRead16"] = (void*)blkRead16;
	jit_symbols["blkRead32"] = (void*)blkRead32;
	jit_symbols["blkRead64"] = (void*)blkRead64;
	jit_symbols["cpuWrite8"] = (void*)cpuWrite8;
	jit_symbols["cpuWrite16"] = (void*)cpuWrite16;
	jit_symbols["cpuWrite32"] = (void*)cpuWrite32;
	jit_symbols["cpuWrite64"] = (void*)cpuWrite64;

	jit_symbols["cpuTraceOnlyMemWrite8"] = (void*)cpuTraceOnlyMemWrite8;
	jit_symbols["cpuTraceOnlyMemWrite16"] = (void*)cpuTraceOnlyMemWrite16;
	jit_symbols["cpuTraceOnlyMemWrite32"] = (void*)cpuTraceOnlyMemWrite32;
	jit_symbols["cpuTraceOnlyMemWrite64"] = (void*)cpuTraceOnlyMemWrite64;

	jit_symbols["cpuTraceOnlyMemRead8"] = (void*)cpuTraceOnlyMemRead8;
	jit_symbols["cpuTraceOnlyMemRead16"] = (void*)cpuTraceOnlyMemRead16;
	jit_symbols["cpuTraceOnlyMemRead32"] = (void*)cpuTraceOnlyMemRead32;
	jit_symbols["cpuTraceOnlyMemRead64"] = (void*)cpuTraceOnlyMemRead64;

	jit_symbols["cpuTraceRegBankWrite"] = (void*)cpuTraceRegBankWrite;
	jit_symbols["cpuTraceRegWrite"] = (void*)cpuTraceRegWrite;
	jit_symbols["cpuTraceRegBankRead"] = (void*)cpuTraceRegBankRead;
	jit_symbols["cpuTraceRegRead"] = (void*)cpuTraceRegRead;

	jit_symbols["cpuTraceInstruction"] = (void*)cpuTraceInstruction;
	jit_symbols["cpuTraceInsnEnd"] = (void*)cpuTraceInsnEnd;

	jit_symbols["__umodti3"] = (void*)uremi128;
	jit_symbols["__udivti3"] = (void*)udivi128;
	jit_symbols["__modti3"] = (void*)remi128;
	jit_symbols["__divti3"] = (void*)divi128;

	// todo: change these to actual bitcode
	jit_symbols["txln_shunt___builtin_f32_is_snan"] = (void*)shunt_builtin_f32_is_snan;
	jit_symbols["txln_shunt___builtin_f32_is_qnan"] = (void*)shunt_builtin_f32_is_qnan;
	jit_symbols["txln_shunt___builtin_f64_is_snan"] = (void*)shunt_builtin_f64_is_snan;
	jit_symbols["txln_shunt___builtin_f64_is_qnan"] = (void*)shunt_builtin_f64_is_qnan;

	auto resolver = llvm::orc::createLambdaResolver(
	[&](const std::string &name) {
		// first, check our internal symbols
		if(jit_symbols.count(name)) {
			return llvm::JITSymbol((intptr_t)jit_symbols.at(name), llvm::JITSymbolFlags::Exported);
		}

		// then check more generally
		if(auto sym = compiler_.findSymbol(name, false)) {
			return sym;
		}

		// we couldn't find the symbol
		return llvm::JITSymbol(nullptr);
	},
	[jit_symbols](const std::string &name) {
		if(jit_symbols.count(name)) {
			return llvm::JITSymbol((intptr_t)jit_symbols.at(name), llvm::JITSymbolFlags::Exported);
		} else return llvm::JITSymbol(nullptr);
	}
	                );

	std::string function_name = function->getName();

	auto handle = compiler_.addModule(std::unique_ptr<llvm::Module>(module), std::move(resolver));
	if(!handle) {
		return nullptr;
	}

	auto symbol = compiler_.findSymbolIn(handle.get(), function_name, true);

	auto address = symbol.getAddress();
	if(!address) {
		return nullptr;
	}


	LLVMTranslation *txln = new LLVMTranslation((LLVMTranslation::translation_fn)address.get(), nullptr);
	for(auto i : unit.GetBlocks()) {
		if(i.second->IsEntryBlock()) {
			txln->AddContainedBlock(i.first);
		}
	}

	if(archsim::options::Debug) {
		std::ofstream str("code_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()));
		str.write((char*)address.get(), memory_manager_->getAllocatedCodeSize((void*)address.get()));
	}

	return txln;
}

void BuildJumpTable(gensim::BaseLLVMTranslate *txlt, TranslationWorkUnit &unit, llvm::Function *function, std::map<archsim::Address, llvm::BasicBlock*> &blocks, llvm::BasicBlock *dispatch_block, llvm::BasicBlock *exit_block)
{
	llvm::IRBuilder<> builder(dispatch_block);
	tx_llvm::LLVMTranslationContext ctx(function->getContext(), function->getParent(), unit.GetThread());
	ctx.SetBuilder(builder);

	auto arraytype = llvm::ArrayType::get(ctx.Types.i8Ptr, 4096);
	std::map<archsim::Address, llvm::BlockAddress*> ptrs;

	std::set<llvm::BasicBlock*> entry_blocks;
	for(auto b : unit.GetBlocks()) {
		if(b.second->IsEntryBlock()) {
			entry_blocks.insert(blocks.at(b.first));
			auto block_address = llvm::BlockAddress::get(function, blocks.at(b.first));
			ptrs[b.first.PageOffset()] = block_address;
		}
	}

	auto exit_block_ptr = llvm::BlockAddress::get(function, exit_block);

	std::vector<llvm::Constant *> constants;
	for(archsim::Address a = archsim::Address(0); a < archsim::Address(0x1000); a += 1) {
		if(ptrs.count(a.PageOffset())) {
			constants.push_back(ptrs.at(a));
		} else {
			constants.push_back(exit_block_ptr);
		}
	}

	llvm::ConstantArray *block_jump_table = (llvm::ConstantArray*)llvm::ConstantArray::get(arraytype, constants);

	llvm::Constant *constant = function->getParent()->getOrInsertGlobal("jump_table", arraytype);
	llvm::GlobalVariable *gv = function->getParent()->getGlobalVariable("jump_table");
	gv->setConstant(true);
	gv->setInitializer(block_jump_table);

	auto pc = txlt->EmitRegisterRead(ctx, 8, 128);

	// need to make sure we're still on the right page
	auto pc_check_less = ctx.GetBuilder().CreateICmpULT(pc, llvm::ConstantInt::get(ctx.Types.i64, unit.GetRegion().GetPhysicalBaseAddress().Get()));
	auto pc_check_greater = ctx.GetBuilder().CreateICmpUGT(pc, llvm::ConstantInt::get(ctx.Types.i64, unit.GetRegion().GetPhysicalBaseAddress().Get() + 4096));
	auto pc_check = ctx.GetBuilder().CreateOr(pc_check_less, pc_check_greater);

	llvm::BasicBlock *on_page_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "jump_table_block", function);
	ctx.GetBuilder().CreateCondBr(pc_check, exit_block, on_page_block);

	ctx.SetBlock(on_page_block);

	auto pc_offset = builder.CreateAnd(pc, archsim::Address::PageMask);
	auto target = ctx.GetBuilder().CreateGEP(gv, {llvm::ConstantInt::get(ctx.Types.i64, 0), pc_offset});
	target = ctx.GetBuilder().CreateLoad(target);
	auto indirectbr = ctx.GetBuilder().CreateIndirectBr(target);

	for(auto i : entry_blocks) {
		// todo: entry blocks only
		indirectbr->addDestination(i);
	}
	indirectbr->addDestination(exit_block);
}

void BuildSwitchStatement(gensim::BaseLLVMTranslate *txlt, TranslationWorkUnit &unit, llvm::Function *function, std::map<archsim::Address, llvm::BasicBlock*> &blocks, llvm::BasicBlock *dispatch_block, llvm::BasicBlock *exit_block)
{
	std::set<archsim::Address> entry_blocks;
	for(auto b : unit.GetBlocks()) {
		if(b.second->IsEntryBlock()) {
			entry_blocks.insert(b.first);
		}
	}

	llvm::IRBuilder<> builder(dispatch_block);
	tx_llvm::LLVMTranslationContext ctx(function->getContext(), function->getParent(), unit.GetThread());
	ctx.SetBuilder(builder);

	auto pc = txlt->EmitRegisterRead(ctx, 8, 128);

	auto switch_inst = builder.CreateSwitch(pc, exit_block, blocks.size());
	for(auto block : blocks) {
		if(entry_blocks.count(block.first)) {
			switch_inst->addCase(llvm::ConstantInt::get(llvm::Type::getInt64Ty(function->getContext()), unit.GetRegion().GetPhysicalBaseAddress().Get() + block.first.GetPageOffset()), block.second);
		}
	}
}

llvm::BasicBlock *BuildDispatchBlock(gensim::BaseLLVMTranslate *txlt, TranslationWorkUnit &unit, llvm::Function *function, std::map<archsim::Address, llvm::BasicBlock*> &blocks, llvm::BasicBlock *exit_block)
{
	auto *entry_block = &function->getEntryBlock();
	for(auto b : unit.GetBlocks()) {
		auto block_block = llvm::BasicBlock::Create(function->getContext(), "block_" + std::to_string(b.first.Get()), function);
		blocks[b.first] = block_block;
	}

	llvm::BasicBlock *dispatch_block = llvm::BasicBlock::Create(function->getContext(), "dispatch", function);
	llvm::BranchInst::Create(dispatch_block, entry_block);

	BuildJumpTable(txlt, unit, function, blocks, dispatch_block, exit_block);

	return dispatch_block;
}

void EmitControlFlow_FallOffPage(tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, std::map<archsim::Address, llvm::BasicBlock*> &block_map, llvm::BasicBlock *dispatch_block)
{
	ctx.GetBuilder().CreateRetVoid();
}

void EmitControlFlow_DirectNonPred(tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, std::map<archsim::Address, llvm::BasicBlock*> &block_map, llvm::BasicBlock *dispatch_block, gensim::JumpInfo &ji)
{
	llvm::BasicBlock *taken_block = dispatch_block;

	if(ji.JumpTarget >= unit.GetRegion().GetPhysicalBaseAddress() && ji.JumpTarget < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		// jump target is on page
		if(block_map.count(ji.JumpTarget.PageOffset())) {
			taken_block = block_map.at(ji.JumpTarget.PageOffset());
		}
	}

	ctx.GetBuilder().CreateBr(taken_block);
}

void EmitControlFlow_DirectPred(tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, std::map<archsim::Address, llvm::BasicBlock*> &block_map, llvm::BasicBlock *dispatch_block, gensim::JumpInfo &ji)
{
	llvm::BasicBlock *taken_block = dispatch_block;
	llvm::BasicBlock *nontaken_block = dispatch_block;

	if(ji.JumpTarget >= unit.GetRegion().GetPhysicalBaseAddress() && ji.JumpTarget < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		// jump target is on page
		archsim::Address offset = ji.JumpTarget.PageOffset();
		if(block_map.count(offset)) {
			taken_block = block_map.at(offset);
		}
	}

	auto fallthrough_addr = unit.GetRegion().GetPhysicalBaseAddress() + ctrlflow->GetOffset() + ctrlflow->GetDecode().Instr_Length;
	if(fallthrough_addr >= unit.GetRegion().GetPhysicalBaseAddress() && fallthrough_addr < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		if(block_map.count(fallthrough_addr.PageOffset())) {
			nontaken_block = block_map.at(fallthrough_addr.PageOffset());
		}
	}

	llvm::Value *pc = translate->EmitRegisterRead(ctx, 8, 128);
	auto branch_was_taken = ctx.GetBuilder().CreateICmpEQ(pc, llvm::ConstantInt::get(ctx.Types.i64, ji.JumpTarget.Get()));
	ctx.GetBuilder().CreateCondBr(branch_was_taken, taken_block, nontaken_block);
}

void EmitControlFlow_Indirect(tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, std::map<archsim::Address, llvm::BasicBlock*> &block_map, llvm::BasicBlock *dispatch_block)
{
	auto pc_val = translate->EmitRegisterRead(ctx, 8, 128);
	llvm::SwitchInst *out_switch = ctx.GetBuilder().CreateSwitch(pc_val, dispatch_block);
	for(auto &succ : block.GetSuccessors()) {
		auto succ_addr = (unit.GetRegion().GetPhysicalBaseAddress() + succ->GetOffset());

		if(block_map.count(succ->GetOffset())) {
			out_switch->addCase(llvm::ConstantInt::get(ctx.Types.i64, succ_addr.Get()), block_map.at(succ->GetOffset()));
		}
	}
}


void EmitControlFlow(tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, std::map<archsim::Address, llvm::BasicBlock*> &block_map, llvm::BasicBlock *dispatch_block)
{
	// several options here:
	// 1. an instruction which isn't a jump but happens to be at the end of a block (due to being at the end of the page): return since we can't do anything
	// 2. a direct non predicated jump: br directly to target block if it is on this page and has been translated
	// 3. a direct predicated jump: check the PC and either branch to the target block (if it's on this page and has been translated) or to the fallthrough block (if on this page and translated)
	// 4. an indirect jump: create a switch statement for profiled branch targets, and add the dispatch block to it.

	auto decode = &ctrlflow->GetDecode();
	auto insn_pc = unit.GetRegion().GetPhysicalBaseAddress() + ctrlflow->GetOffset();

	gensim::JumpInfo ji;
	auto jip = unit.GetThread()->GetArch().GetISA(decode->isa_mode).GetNewJumpInfo();
	jip->GetJumpInfo(decode, insn_pc, ji);

	if(!ji.IsJump) {
		// situation 1
		EmitControlFlow_FallOffPage(ctx, translate, unit, block, ctrlflow, block_map, dispatch_block);
	} else {
		if(!ji.IsIndirect) {
			if(!ji.IsConditional) {
				// situation 2
				EmitControlFlow_DirectNonPred(ctx, translate, unit, block, ctrlflow, block_map, dispatch_block, ji);
			} else {
				// situation 3
				EmitControlFlow_DirectPred(ctx, translate, unit, block, ctrlflow, block_map, dispatch_block, ji);
			}
		} else {
			// situation 4
			EmitControlFlow_Indirect(ctx, translate, unit, block, ctrlflow, block_map, dispatch_block);
		}
	}

	delete jip;
}

void TranslateInstructionPredicated(tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address insn_pc, llvm::Function *fn)
{
	gensim::JumpInfo ji;
	auto jip = thread->GetArch().GetISA(decode->isa_mode).GetNewJumpInfo();
	jip->GetJumpInfo(decode, insn_pc, ji);

	llvm::Value *predicate_passed = translate->EmitPredicateCheck(ctx, thread, decode, insn_pc, fn);
	predicate_passed = ctx.GetBuilder().CreateICmpNE(predicate_passed, llvm::ConstantInt::get(predicate_passed->getType(), 0));

	llvm::BasicBlock *passed_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);
	llvm::BasicBlock *continue_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);
	llvm::BasicBlock *failed_block = continue_block;

	if(ji.IsJump) {
		auto prev_block = ctx.GetBuilder().GetInsertBlock();

		// if we're translating a predicated jump, we need to increment the PC only if the predicate fails
		failed_block = llvm::BasicBlock::Create(ctx.LLVMCtx, "", fn);

		ctx.SetBlock(failed_block);
		translate->EmitRegisterWrite(ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get() + decode->Instr_Length));
		ctx.GetBuilder().CreateBr(continue_block);
		ctx.SetBlock(prev_block);
	}

	ctx.GetBuilder().CreateCondBr(predicate_passed, passed_block, failed_block);
	ctx.SetBlock(passed_block);
	translate->TranslateInstruction(ctx, thread, decode, insn_pc, fn);
	ctx.GetBuilder().CreateBr(continue_block);

	ctx.SetBlock(continue_block);
	// update PC

	if(!ji.IsJump) {
		translate->EmitRegisterWrite(ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get() + decode->Instr_Length));
	}

	delete jip;
}

/**
 * Translates the given translation work unit.
 * @param llvm_ctx The LLVM context to perform the translation with.
 * @param unit The unit to translate.
 */
void AsynchronousTranslationWorker::Translate(::llvm::LLVMContext& llvm_ctx, TranslationWorkUnit& unit)
{
	// If debugging is turned on, dump the control-flow graph for this work unit.
	if (archsim::options::Debug) {
		unit.DumpGraph();
	}

	if (archsim::options::Verbose) {
		units.inc();
		blocks.inc(unit.GetBlockCount());
	}

	LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translating: " << unit;

	// Create a new LLVM translation context.
	LLVMOptimiser opt;

	// Create a new llvm module to contain the translation
	llvm::Module *module = new llvm::Module("region_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()), llvm_ctx);
	module->setDataLayout(target_machine_->createDataLayout());

	auto i8ptrty = llvm::Type::getInt8PtrTy(llvm_ctx);
	llvm::FunctionType *fn_type = llvm::FunctionType::get(llvm::Type::getVoidTy(llvm_ctx), {i8ptrty, i8ptrty}, false);
	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction("fn", fn_type);
	auto entry_block = llvm::BasicBlock::Create(llvm_ctx, "entry_block", fn);
	auto exit_block = llvm::BasicBlock::Create(llvm_ctx, "exit_block", fn);

	gensim::BaseLLVMTranslate *translate = translate_;
//	translate->InitialiseFeatures(unit.GetThread());
//	translate->InitialiseIsaMode(unit.GetThread());
//	translate->SetDecodeContext(unit.GetThread()->GetEmulationModel().GetNewDecodeContext(*unit.GetThread()));

	std::map<Address, llvm::BasicBlock*> block_map;

	auto ji = unit.GetThread()->GetArch().GetISA(0).GetNewJumpInfo();

	llvm::BasicBlock *dispatch_block = BuildDispatchBlock(translate, unit, fn, block_map, exit_block);
	tx_llvm::LLVMTranslationContext ctx(module->getContext(), module, unit.GetThread());

	for(auto block : unit.GetBlocks()) {
		llvm::BasicBlock *startblock = block_map.at(block.first);
		llvm::IRBuilder<> builder(startblock);
		ctx.SetBuilder(builder);
		ctx.SetBlock(startblock);

		if(archsim::options::Verbose) {
			// increment instruction counter
			translate->EmitIncrementCounter(ctx, unit.GetThread()->GetMetrics().InstructionCount, block.second->GetInstructions().size());
			translate->EmitIncrementCounter(ctx, unit.GetThread()->GetMetrics().JITInstructionCount, block.second->GetInstructions().size());
		}

		for(auto insn : block.second->GetInstructions()) {
			auto insn_pc = unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset();
			if(archsim::options::Trace) {
				builder.CreateCall(ctx.Functions.cpuTraceInstruction, {ctx.GetThreadPtr(), llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get()), llvm::ConstantInt::get(ctx.Types.i32, insn->GetDecode().ir), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 1)});
			}


			if(insn->GetDecode().GetIsPredicated()) {
				TranslateInstructionPredicated(ctx, translate, unit.GetThread(), &insn->GetDecode(), Address(unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset()), fn);
			} else {
				translate->TranslateInstruction(ctx, unit.GetThread(), &insn->GetDecode(), Address(unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset()), fn);

				gensim::JumpInfo jumpinfo;
				ji->GetJumpInfo(&insn->GetDecode(), Address(0), jumpinfo);
				if(!jumpinfo.IsJump) {
					translate_->EmitRegisterWrite(ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, (insn->GetOffset() + unit.GetRegion().GetPhysicalBaseAddress() + insn->GetDecode().Instr_Length).Get()));
				}

			}
			ctx.ResetRegisters();


			if(archsim::options::Trace) {
				builder.CreateCall(ctx.Functions.cpuTraceInsnEnd, {ctx.GetThreadPtr()});
			}
		}

		// did we have a direct or indirect jump? if direct, then assume the jump is conditional
		EmitControlFlow(ctx, translate_, unit, *block.second, block.second->GetInstructions().back(), block_map, dispatch_block);

	}

	llvm::ReturnInst::Create(llvm_ctx, nullptr, exit_block);

	if(archsim::options::Debug) {
		std::ofstream str("fn_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()));
		llvm::raw_os_ostream llvm_str (str);
		module->print(llvm_str, nullptr);
	}

	// optimise
	opt.Optimise(module, target_machine_->createDataLayout());


	if(archsim::options::Debug) {
		std::ofstream str("fn_opt_" + std::to_string(unit.GetRegion().GetPhysicalBaseAddress().Get()));
		llvm::raw_os_ostream llvm_str (str);
		module->print(llvm_str, nullptr);
	}

	// compile
	auto txln = CompileModule(unit, module, fn);

	if (!txln) {
		LC_ERROR(LogTranslate) << "[" << (uint32_t)id << "] Translation Failed: " << unit;
	} else {
		LC_DEBUG2(LogTranslate) << "[" << (uint32_t)id << "] Translation Succeeded: " << unit;

		if (archsim::options::Verbose) {
			txlns.inc();
		}

		if (!mgr.MarkTranslationAsComplete(unit.GetRegion(), *txln)) {
			delete txln;
		}
	}
}

void AsynchronousTranslationWorker::PrintStatistics(std::ostream& stream)
{
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.generation.GetElapsedS();
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.optimisation.GetElapsedS();
	stream << std::fixed << std::left << std::setprecision(2) << std::setw(14) << timers.compilation.GetElapsedS();
	stream << std::endl;
}
