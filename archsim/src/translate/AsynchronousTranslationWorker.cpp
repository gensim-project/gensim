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

void EmitControlFlow_FallOffPage(llvm::IRBuilder<> &builder, tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, tx_llvm::LLVMRegionTranslationContext &region)
{
	builder.CreateBr(region.GetExitBlock(tx_llvm::LLVMRegionTranslationContext::EXIT_REASON_NEXTPAGE));
}

void EmitControlFlow_DirectNonPred(llvm::IRBuilder<> &builder, tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, tx_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
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

void EmitControlFlow_DirectPred(llvm::IRBuilder<> &builder, tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, tx_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
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

	auto fallthrough_addr = unit.GetRegion().GetPhysicalBaseAddress() + ctrlflow->GetOffset() + ctrlflow->GetDecode().Instr_Length;
	if(fallthrough_addr >= unit.GetRegion().GetPhysicalBaseAddress() && fallthrough_addr < unit.GetRegion().GetPhysicalBaseAddress() + 4096) {
		if(region.GetBlockMap().count(fallthrough_addr.PageOffset())) {
			nontaken_block = region.GetBlockMap().at(fallthrough_addr.PageOffset());
		}
	}

	llvm::Value *pc = translate->EmitRegisterRead(builder, ctx, 8, 128);
	auto branch_was_taken = builder.CreateICmpEQ(pc, llvm::ConstantInt::get(ctx.Types.i64, ji.JumpTarget.Get()));
	builder.CreateCondBr(branch_was_taken, taken_block, nontaken_block);
}

void EmitControlFlow_Indirect(llvm::IRBuilder<> &builder, tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, tx_llvm::LLVMRegionTranslationContext &region, gensim::JumpInfo &ji)
{
	auto pc_val = translate->EmitRegisterRead(builder, ctx, 8, 128);
	llvm::SwitchInst *out_switch = builder.CreateSwitch(pc_val, region.GetDispatchBlock());
	for(auto &succ : block.GetSuccessors()) {
		auto succ_addr = (unit.GetRegion().GetPhysicalBaseAddress() + succ->GetOffset());

		if(region.GetBlockMap().count(succ->GetOffset())) {
			out_switch->addCase(llvm::ConstantInt::get(ctx.Types.i64, succ_addr.Get()), region.GetBlockMap().at(succ->GetOffset()));
		}
	}
}


void EmitControlFlow(llvm::IRBuilder<> &builder, tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, TranslationWorkUnit &unit, TranslationBlockUnit &block, TranslationInstructionUnit *ctrlflow, tx_llvm::LLVMRegionTranslationContext &region)
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

void TranslateInstructionPredicated(llvm::IRBuilder<> &builder, tx_llvm::LLVMTranslationContext &ctx, gensim::BaseLLVMTranslate *translate, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address insn_pc, llvm::Function *fn)
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

		translate->EmitRegisterWrite(failed_builder, ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get() + decode->Instr_Length));
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
		translate->EmitRegisterWrite(builder, ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get() + decode->Instr_Length));
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
	unit.GetRegion().SetStatus(Region::InTranslation);
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
	module->setTargetTriple(target_machine_->getTargetTriple().normalize());

	auto i8ptrty = llvm::Type::getInt8PtrTy(llvm_ctx);

	llvm::FunctionType *fn_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_ctx), {i8ptrty, i8ptrty}, false);

	llvm::Function *fn = (llvm::Function*)module->getOrInsertFunction("fn", fn_type);
	fn->addParamAttr(0, llvm::Attribute::NoCapture);
	fn->addParamAttr(0, llvm::Attribute::NoAlias);

	auto entry_block = llvm::BasicBlock::Create(llvm_ctx, "entry_block", fn);

	gensim::BaseLLVMTranslate *translate = translate_;
//	translate->InitialiseFeatures(unit.GetThread());
//	translate->InitialiseIsaMode(unit.GetThread());
//	translate->SetDecodeContext(unit.GetThread()->GetEmulationModel().GetNewDecodeContext(*unit.GetThread()));

	auto ji = unit.GetThread()->GetArch().GetISA(0).GetNewJumpInfo();

	tx_llvm::LLVMTranslationContext ctx(module->getContext(), fn, unit.GetThread());

	tx_llvm::LLVMRegionTranslationContext region_ctx(ctx, fn, unit, entry_block, translate);

	for(auto block : unit.GetBlocks()) {
		llvm::BasicBlock *startblock = region_ctx.GetBlockMap().at(block.first);
		llvm::IRBuilder<> builder(startblock);

		if(archsim::options::Verbose) {
			// increment instruction counter
			translate->EmitIncrementCounter(builder, ctx, unit.GetThread()->GetMetrics().InstructionCount, block.second->GetInstructions().size());
			translate->EmitIncrementCounter(builder, ctx, unit.GetThread()->GetMetrics().JITInstructionCount, block.second->GetInstructions().size());
		}

		for(auto insn : block.second->GetInstructions()) {
			auto insn_pc = unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset();

			if(insn->GetDecode().GetIsPredicated()) {
				TranslateInstructionPredicated(builder, ctx, translate, unit.GetThread(), &insn->GetDecode(), Address(unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset()), fn);
			} else {
				if(archsim::options::Trace) {
					builder.CreateCall(ctx.Functions.cpuTraceInstruction, {ctx.GetThreadPtr(builder), llvm::ConstantInt::get(ctx.Types.i64, insn_pc.Get()), llvm::ConstantInt::get(ctx.Types.i32, insn->GetDecode().ir), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 0), llvm::ConstantInt::get(ctx.Types.i32, 1)});
				}

				translate->TranslateInstruction(builder, ctx, unit.GetThread(), &insn->GetDecode(), Address(unit.GetRegion().GetPhysicalBaseAddress() + insn->GetOffset()), fn);

				if(archsim::options::Trace) {
					builder.CreateCall(ctx.Functions.cpuTraceInsnEnd, {ctx.GetThreadPtr(builder)});
				}

				gensim::JumpInfo jumpinfo;
				ji->GetJumpInfo(&insn->GetDecode(), Address(0), jumpinfo);
				if(!jumpinfo.IsJump) {
					translate_->EmitRegisterWrite(builder, ctx, 8, 128, llvm::ConstantInt::get(ctx.Types.i64, (insn->GetOffset() + unit.GetRegion().GetPhysicalBaseAddress() + insn->GetDecode().Instr_Length).Get()));
				}

			}
			ctx.ResetRegisters();

		}

		// did we have a direct or indirect jump? if direct, then assume the jump is conditional
		EmitControlFlow(builder, ctx, translate_, unit, *block.second, block.second->GetInstructions().back(), region_ctx);

	}

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
