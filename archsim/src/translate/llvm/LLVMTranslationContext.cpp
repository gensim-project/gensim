/*
 * LLVMTranslationContext.cpp
 */
#include "translate/llvm/LLVMTranslationContext.h"
#include "translate/llvm/LLVMOptimiser.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/profile/Block.h"
#include "translate/llvm/LLVMTranslation.h"
#include "translate/llvm/LLVMMemoryManager.h"

#include "abi/memory/MemoryTranslationModel.h"

#include "abi/memory/MemoryEventHandlerTranslator.h"

#include "gensim/gensim_decode.h"
#include "gensim/gensim_translate.h"

#include "util/LogContext.h"

#include "translate/jit_funs.h"
#include "system.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>

#ifdef LLVM_LATEST
# include <llvm/Linker/Linker.h>
# include <llvm/IR/IRPrintingPasses.h>
#else
# include <llvm/Linker/Linker.h>
#include <llvm/IR/IRPrintingPasses.h>
#endif

#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitstreamReader.h>

#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_os_ostream.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>

#include <llvm/Transforms/IPO.h>

#include <vector>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <sys/stat.h>

#define BLOCK_ALIGN_BITS 2

UseLogContext(LogTranslate);
UseLogContext(LogAliasAnalysis);

namespace archsim
{
	namespace translate
	{
		template<>
		archsim::translate::RegionTranslationContext<archsim::translate::translate_llvm::LLVMTranslationContext>::RegionTranslationContext(archsim::translate::translate_llvm::LLVMTranslationContext& parent) : txln(parent) { }

		template<>
		archsim::translate::BlockTranslationContext<archsim::translate::translate_llvm::LLVMRegionTranslationContext>::BlockTranslationContext(archsim::translate::translate_llvm::LLVMRegionTranslationContext& parent, archsim::translate::TranslationBlockUnit& tbu) : region(parent), tbu(tbu) { }

		template<>
		archsim::translate::BlockTranslationContext<archsim::translate::translate_llvm::LLVMRegionTranslationContext>::~BlockTranslationContext() {}

		template<>
		archsim::translate::InstructionTranslationContext<archsim::translate::translate_llvm::LLVMBlockTranslationContext>::InstructionTranslationContext(archsim::translate::translate_llvm::LLVMBlockTranslationContext& parent, archsim::translate::TranslationInstructionUnit& tiu) : block(parent), tiu(tiu) { }

		template<>
		archsim::translate::InstructionTranslationContext<archsim::translate::translate_llvm::LLVMBlockTranslationContext>::~InstructionTranslationContext() {}


	}
}



archsim::translate::translate_llvm::LLVMTranslationContext::LLVMTranslationContext(archsim::translate::TranslationManager& tmgr, archsim::translate::TranslationWorkUnit& twu, ::llvm::LLVMContext& llvm_ctx, LLVMOptimiser &opt, archsim::util::PagePool &code_pool)
	: archsim::translate::TranslationContext(tmgr, twu),
	  llvm_ctx(llvm_ctx),
	  optimiser(opt),
	  gensim_translate(nullptr),
	  code_pool(code_pool)
{
	gensim_translate = nullptr; //(gensim::BaseLLVMTranslate *)twu.GetThread().CreateLLVMTranslate(*this);

	Initialise();
}

archsim::translate::translate_llvm::LLVMTranslationContext::~LLVMTranslationContext()
{
	delete gensim_translate;
}

archsim::translate::translate_llvm::LLVMRegionTranslationContext::LLVMRegionTranslationContext(archsim::translate::translate_llvm::LLVMTranslationContext& parent, ::llvm::IRBuilder<>& builder) : RegionTranslationContext(parent), builder(builder) { }

archsim::translate::translate_llvm::LLVMBlockTranslationContext::LLVMBlockTranslationContext(archsim::translate::translate_llvm::LLVMRegionTranslationContext& parent, archsim::translate::TranslationBlockUnit& tbu, ::llvm::BasicBlock *llvm_block) : BlockTranslationContext(parent, tbu), llvm_block(llvm_block) { }

archsim::translate::translate_llvm::LLVMInstructionTranslationContext::LLVMInstructionTranslationContext(archsim::translate::translate_llvm::LLVMBlockTranslationContext& parent, archsim::translate::TranslationInstructionUnit& tiu) : InstructionTranslationContext(parent, tiu) { }

void archsim::translate::translate_llvm::LLVMInstructionTranslationContext::AddAliasAnalysisNode(::llvm::Instruction* insn, archsim::translate::translate_llvm::AliasAnalysisTag tag)
{
	block.region.txln.AddAliasAnalysisNode(insn, tag);
}

using namespace archsim::translate::translate_llvm;

void LLVMTranslationContext::Initialise()
{
	if (gensim_translate->GetPrecompSize() == 0) {
		LC_ERROR(LogTranslate) << "Processor module is missing LLVM translations";
		assert(false);
	}

	auto code_buffer = ::llvm::MemoryBuffer::getMemBuffer(::llvm::StringRef((char *)gensim_translate->GetPrecompBitcode(), gensim_translate->GetPrecompSize()), "PrecompModule", false);
	if (code_buffer == nullptr) {
		LC_ERROR(LogTranslate) << "Could not load precomp module";
		assert(false);
	}

	llvm_module = new ::llvm::Module("jit", llvm_ctx);

	bzero((void *)&types, sizeof(types));
	bzero((void *)&jit_functions, sizeof(jit_functions));

	types.vtype = ::llvm::Type::getVoidTy(llvm_ctx);

	types.i1 = ::llvm::IntegerType::getInt1Ty(llvm_ctx);
	types.i8 = ::llvm::IntegerType::getInt8Ty(llvm_ctx);
	types.i16 = ::llvm::IntegerType::getInt16Ty(llvm_ctx);
	types.i32 = ::llvm::IntegerType::getInt32Ty(llvm_ctx);
	types.i64 = ::llvm::IntegerType::getInt64Ty(llvm_ctx);

	types.f32 = ::llvm::Type::getFloatTy(llvm_ctx);
	types.f64 = ::llvm::Type::getDoubleTy(llvm_ctx);

	types.pi1 = ::llvm::IntegerType::getInt1PtrTy(llvm_ctx);
	types.pi8 = ::llvm::IntegerType::getInt8PtrTy(llvm_ctx);
	types.pi16 = ::llvm::IntegerType::getInt16PtrTy(llvm_ctx);
	types.pi32 = ::llvm::IntegerType::getInt32PtrTy(llvm_ctx);
	types.pi64 = ::llvm::IntegerType::getInt64PtrTy(llvm_ctx);

	types.state = llvm_module->getTypeByName("struct.cpuState");
	types.state_ptr = ::llvm::PointerType::get(types.state, 0);

	types.reg_state = llvm_module->getTypeByName("struct.gensim_state");
	types.reg_state_ptr = ::llvm::PointerType::get(types.reg_state, 0);

	types.cpu_ctx = types.state->getContainedType(0);

	::llvm::Type *stackmap_params[] { types.i32, types.i32 };
	::llvm::FunctionType *stackmap_fn_ty = ::llvm::FunctionType::get(types.vtype, stackmap_params, true);

	intrinsics.stack_map = (::llvm::Function *)llvm_module->getOrInsertFunction("llvm.experimental.stackmap", stackmap_fn_ty);

	jit_functions.debug0 = (::llvm::Function *)llvm_module->getOrInsertFunction("jitDebug0", types.i32, nullptr);
	jit_functions.debug1 = (::llvm::Function *)llvm_module->getOrInsertFunction("jitDebug1", types.i32, types.i64, nullptr);
	jit_functions.debug2 = (::llvm::Function *)llvm_module->getOrInsertFunction("jitDebug2", types.i32, types.i64, types.i64, nullptr);
	jit_functions.debug_cpu = (::llvm::Function *)llvm_module->getOrInsertFunction("jitDebugCpu", types.i32, types.cpu_ctx, nullptr);
	jit_functions.jit_trap = (::llvm::Function *)llvm_module->getOrInsertFunction("jitTrap", types.vtype, nullptr);
	jit_functions.jit_trap_if = (::llvm::Function *)llvm_module->getOrInsertFunction("jitTrapIf", types.vtype, types.i1, nullptr);
	jit_functions.jit_assert = (::llvm::Function *)llvm_module->getOrInsertFunction("jitAssert", types.vtype, types.i1, nullptr);

	jit_functions.cpu_translate = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTranslate", types.i32, types.cpu_ctx, types.i32, types.pi32, nullptr);
	jit_functions.cpu_handle_pending_action = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuHandlePendingAction", types.i32, types.cpu_ctx, nullptr);
	jit_functions.cpu_return_to_safepoint = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuReturnToSafepoint", types.vtype, types.cpu_ctx, nullptr);
//	jit_functions.cpu_return_to_safepoint->setDoesNotReturn();

	jit_functions.cpu_exec_mode = (::llvm::Function*)llvm_module->getOrInsertFunction("cpuGetExecMode", types.i8, types.cpu_ctx, nullptr);

	jit_functions.cpu_enter_kernel = (::llvm::Function*)llvm_module->getOrInsertFunction("cpuEnterKernelMode", types.cpu_ctx, nullptr);
	jit_functions.cpu_enter_user = (::llvm::Function*)llvm_module->getOrInsertFunction("cpuEnterUserMode", types.cpu_ctx, nullptr);

	jit_functions.cpu_read_8 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuRead8", types.i32, types.cpu_ctx, types.i32, types.pi8, nullptr);
	jit_functions.cpu_read_16 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuRead16", types.i32, types.cpu_ctx, types.i32, types.pi16, nullptr);
	jit_functions.cpu_read_32 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuRead32", types.i32, types.cpu_ctx, types.i32, types.pi32, nullptr);

	jit_functions.cpu_write_8 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuWrite8", types.i32, types.cpu_ctx, types.i32, types.i8, nullptr);
	jit_functions.cpu_write_16 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuWrite16", types.i32, types.cpu_ctx, types.i32, types.i16, nullptr);
	jit_functions.cpu_write_32 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuWrite32", types.i32, types.cpu_ctx, types.i32, types.i32, nullptr);

	jit_functions.mem_write_8 = GetMemWriteFunction(8);
	jit_functions.mem_write_16 = GetMemWriteFunction(16);
	jit_functions.mem_write_32 = GetMemWriteFunction(32);

	jit_functions.mem_read_8 = GetMemReadFunction(8);
	jit_functions.mem_read_16 = GetMemReadFunction(16);
	jit_functions.mem_read_32 = GetMemReadFunction(32);

	jit_functions.cpu_read_8_user = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuRead8User", types.i32, types.cpu_ctx, types.i32, types.pi8, nullptr);
	jit_functions.cpu_read_32_user = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuRead32User", types.i32, types.cpu_ctx, types.i32, types.pi32, nullptr);
	jit_functions.cpu_write_8_user = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuWrite8User", types.i32, types.cpu_ctx, types.i32, types.i8, nullptr);
	jit_functions.cpu_write_32_user = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuWrite32User", types.i32, types.cpu_ctx, types.i32, types.i32, nullptr);

	jit_functions.cpu_take_exception = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTakeException", types.i32, types.cpu_ctx, types.i32, types.i32, nullptr);
	jit_functions.cpu_push_interrupt = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuPushInterruptState", types.vtype, types.cpu_ctx, types.i32, nullptr);
	jit_functions.cpu_pop_interrupt = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuPopInterruptState", types.vtype, types.cpu_ctx, nullptr);
	jit_functions.cpu_pend_irq = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuPendInterrupt", types.vtype, types.cpu_ctx, nullptr);

	jit_functions.cpu_halt = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuHalt", types.vtype, types.cpu_ctx, nullptr);

	jit_functions.dev_probe_device = (::llvm::Function *)llvm_module->getOrInsertFunction("devProbeDevice", types.i8, types.cpu_ctx, types.i32, nullptr);
	jit_functions.dev_read_device = (::llvm::Function *)llvm_module->getOrInsertFunction("devReadDevice", types.i8, types.cpu_ctx, types.i32, types.i32, types.pi32, nullptr);
	jit_functions.dev_write_device = (::llvm::Function *)llvm_module->getOrInsertFunction("devWriteDevice", types.i8, types.cpu_ctx, types.i32, types.i32, types.i32, nullptr);

	jit_functions.tm_flush = (::llvm::Function *)llvm_module->getOrInsertFunction("tmFlush", types.vtype, types.cpu_ctx, nullptr);
	jit_functions.tm_flush_dtlb = (::llvm::Function *)llvm_module->getOrInsertFunction("tmFlushDTlb", types.vtype, types.cpu_ctx, nullptr);
	jit_functions.tm_flush_itlb = (::llvm::Function *)llvm_module->getOrInsertFunction("tmFlushITlb", types.vtype, types.cpu_ctx, nullptr);
	jit_functions.tm_flush_dtlb_entry = (::llvm::Function *)llvm_module->getOrInsertFunction("tmFlushDTlbEntry", types.vtype, types.cpu_ctx, types.i32, nullptr);
	jit_functions.tm_flush_itlb_entry = (::llvm::Function *)llvm_module->getOrInsertFunction("tmFlushITlbEntry", types.vtype, types.cpu_ctx, types.i32, nullptr);

	jit_functions.jit_checksum_page = (::llvm::Function *)llvm_module->getOrInsertFunction("jitChecksumPage", types.i32, types.cpu_ctx, types.i32, nullptr);
	jit_functions.jit_check_checksum = (::llvm::Function *)llvm_module->getOrInsertFunction("jitCheckChecksum", types.vtype, types.i32, types.i32, nullptr);

	jit_functions.double_sqrt = (::llvm::Function *)llvm_module->getOrInsertFunction("llvm.sqrt.f64", types.f64, types.f64, nullptr);
	jit_functions.float_sqrt = (::llvm::Function *)llvm_module->getOrInsertFunction("llvm.sqrt.f32", types.f32, types.f32, nullptr);

	jit_functions.double_abs = (::llvm::Function *)llvm_module->getOrInsertFunction("llvm.fabs.f64", types.f64, types.f64, nullptr);
	jit_functions.float_abs = (::llvm::Function *)llvm_module->getOrInsertFunction("llvm.fabs.f32", types.f32, types.f32, nullptr);

	jit_functions.genc_adc_flags = (::llvm::Function *)llvm_module->getOrInsertFunction("genc_adc_flags", types.i32, types.i32, types.i32, types.i8, nullptr);
	jit_functions.genc_adc_flags->addFnAttr(::llvm::Attribute::ReadNone);
	jit_functions.genc_adc_flags->addFnAttr(::llvm::Attribute::NoUnwind);

	if (twu.ShouldEmitTracing()) {
		jit_functions.trace_start_insn = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceInstruction", types.vtype, types.cpu_ctx, types.i32, types.i32, types.i8, types.i8, types.i8, nullptr);
		jit_functions.trace_end_insn = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceInsnEnd", types.vtype, types.cpu_ctx, nullptr);

		jit_functions.trace_reg_write = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceRegWrite", types.vtype, types.cpu_ctx, types.i8, types.i32, nullptr);
		jit_functions.trace_reg_read = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceRegRead", types.vtype, types.cpu_ctx, types.i8, types.i32, nullptr);

		jit_functions.trace_reg_bank_write = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceRegBankWrite", types.vtype, types.cpu_ctx, types.i8, types.i32, types.i32, nullptr);
		jit_functions.trace_reg_bank_read = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceRegBankRead", types.vtype, types.cpu_ctx, types.i8, types.i32, types.i32, nullptr);

		jit_functions.trace_mem_read_8 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceOnlyMemRead8", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);
		jit_functions.trace_mem_read_16 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceOnlyMemRead16", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);
		jit_functions.trace_mem_read_32 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceOnlyMemRead32", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);

		jit_functions.trace_mem_write_8 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceOnlyMemWrite8", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);
		jit_functions.trace_mem_write_16 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceOnlyMemWrite16", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);
		jit_functions.trace_mem_write_32 = (::llvm::Function *)llvm_module->getOrInsertFunction("cpuTraceOnlyMemWrite32", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);
	}

	jit_functions.cpu_read_pc = (::llvm::Function *)llvm_module->getOrInsertFunction("arm_fast_read_pc", types.i32, types.state_ptr, types.reg_state_ptr, nullptr);
	jit_functions.cpu_write_pc = (::llvm::Function *)llvm_module->getOrInsertFunction("arm_fast_write_pc", types.vtype, types.state_ptr, types.reg_state_ptr, types.i32, nullptr);

	jit_functions.sys_verify = (::llvm::Function *)llvm_module->getOrInsertFunction("sysVerify", types.vtype, types.cpu_ctx, nullptr);
	jit_functions.sys_publish_insn = (::llvm::Function *)llvm_module->getOrInsertFunction("sysPublishInstruction", types.vtype, types.cpu_ctx, types.i32, types.i32, nullptr);
	jit_functions.sys_publish_block = (::llvm::Function *)llvm_module->getOrInsertFunction("sysPublishBlock", types.vtype, types.pi8, types.i32, types.i32, nullptr);

	jit_functions.smart_return = (::llvm::Function *)llvm_module->getOrInsertFunction("SmartReturn", types.vtype, types.i32, types.i32, nullptr);
}

void LLVMTranslationContext::AddAliasAnalysisNode(::llvm::Instruction *insn, AliasAnalysisTag tag)
{
	UNIMPLEMENTED;
	/*
	if (insn->getValueID() <= ::llvm::Value::InstructionVal) {
		return;
	}

	::llvm::SmallVector< ::llvm::Value *, 1> AR;
	AR.push_back(GetConstantInt32(tag));

	::llvm::MDNode* node = ::llvm::MDNode::get(llvm_ctx, AR);
	((::llvm::Instruction *)insn)->setMetadata("aaai", node);
	 */
}

void LLVMTranslationContext::AddAliasAnalysisToRegSlotAccess(::llvm::Value *ptr, uint32_t slot_offset, uint32_t slot_size)
{
	UNIMPLEMENTED;
	
//	::llvm::SmallVector<::llvm::Value*, 4> AR;
//	AR.push_back(::llvm::ConstantInt::get(types.i32, 0, false));
//	AR.push_back(::llvm::ConstantInt::get(types.i32, slot_offset, false));
//	AR.push_back(::llvm::ConstantInt::get(types.i32, slot_offset + slot_size, false));
//	AR.push_back(::llvm::ConstantInt::get(types.i32, slot_size, false));
//	((::llvm::Instruction *)ptr)->setMetadata("aaai", ::llvm::MDNode::get(llvm_ctx, AR));
}

void LLVMTranslationContext::AddAliasAnalysisToRegBankAccess(::llvm::Value *ptr, ::llvm::Value *regnum, uint32_t bank_offset, uint32_t bank_stride, uint32_t bank_elements)
{
	UNIMPLEMENTED;
	
//#define CONSTVAL(a) (assert(a->getValueID() == ::llvm::Instruction::ConstantIntVal), (((::llvm::ConstantInt *)(a))->getZExtValue()))
//#define ISCONSTVAL(a) (a->getValueID() == ::llvm::Instruction::ConstantIntVal)
//
//	uint32_t extent_min = bank_offset;
//	uint32_t extent_max = bank_offset + (bank_stride * bank_elements);
//
//	if(ISCONSTVAL(regnum)) {
//		uint32_t reg_idx = CONSTVAL(regnum);
//		extent_min = bank_offset + (bank_stride * reg_idx);
//		extent_max = bank_offset + (bank_stride * reg_idx) + bank_stride;
//	}
//
//	::llvm::SmallVector<::llvm::Value*, 4> AR;
//	AR.push_back(::llvm::ConstantInt::get(types.i32, 0, false));
//	AR.push_back(::llvm::ConstantInt::get(types.i32, extent_min, false));
//	AR.push_back(::llvm::ConstantInt::get(types.i32, extent_max, false));
//	AR.push_back(::llvm::ConstantInt::get(types.i32, bank_stride, false));
//	((::llvm::Instruction *)ptr)->setMetadata("aaai", ::llvm::MDNode::get(llvm_ctx, AR));
//
//#undef CONSTVAL
//#undef ISCONSTVAL
}

bool LLVMTranslationContext::CreateLLVMFunction(LLVMRegionTranslationContext& rtc)
{
	// Create the function type
	std::vector< ::llvm::Type *> params;
	params.push_back(types.state_ptr);

	::llvm::FunctionType *region_fn_type = ::llvm::FunctionType::get(types.i32, params, false);

	// Generate a name for the region
	std::stringstream region_name;
	region_name << "guest_region_" << std::hex << twu.GetRegion().GetPhysicalBaseAddress();
	rtc.region_fn = ::llvm::Function::Create(region_fn_type, ::llvm::GlobalValue::ExternalLinkage, region_name.str(), llvm_module);

	rtc.region_fn->setCallingConv(::llvm::CallingConv::C);
	rtc.region_fn->addFnAttr(::llvm::Attribute::NoUnwind);

	::llvm::AttrBuilder attrBuilder;
	attrBuilder.addAttribute(::llvm::Attribute::NoAlias);
//	attrBuilder.addAttribute(::llvm::Attribute::get(llvm_ctx, ::llvm::Attribute::Dereferenceable, sizeof(struct cpuState)));

	// Mark the incoming argument as noalias
	rtc.region_fn->args().begin()->addAttrs(attrBuilder);

	return true;
}

bool LLVMTranslationContext::CreateDefaultBlocks(LLVMRegionTranslationContext& rtc)
{
	// Create the default blocks required for operation of the function
	rtc.entry_block = ::llvm::BasicBlock::Create(llvm_ctx, "entry_block", rtc.region_fn);
	rtc.dispatch_block = ::llvm::BasicBlock::Create(llvm_ctx, "dispatch_block", rtc.region_fn);
	rtc.chain_block = ::llvm::BasicBlock::Create(llvm_ctx, "chain_block", rtc.region_fn);
	rtc.interrupt_handler_block = ::llvm::BasicBlock::Create(llvm_ctx, "interrupt_handler", rtc.region_fn);

	return true;
}

::llvm::BasicBlock *LLVMRegionTranslationContext::GetExitBlock(LeaveReasons reason)
{
	if(exit_blocks[reason] == nullptr) CreateExitBlock(reason);
	return exit_blocks[reason];
}

::llvm::BasicBlock *LLVMRegionTranslationContext::GetInstructionExitBlock(LeaveReasons reason)
{
	if(exit_instruction_blocks[reason] == nullptr) CreateInstructionExitBlock(reason);
	return exit_instruction_blocks[reason];
}

bool LLVMRegionTranslationContext::CreateExitBlock(LLVMRegionTranslationContext::LeaveReasons reason)
{
	// First, create the 'bare' block used when leaving outside of an individual instruction
	::llvm::BasicBlock *bare_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "", region_fn);
	::llvm::IRBuilder<> builder (bare_block);

	exit_blocks[reason] = bare_block;

	if (archsim::options::Verbose) {
//		EmitCounterUpdate(builder, txln.twu.GetProcessor().metrics.leave_jit[(int)reason], 1);
	}

	builder.CreateRet(txln.GetConstantInt32((uint32_t)reason));

	return true;
}

bool LLVMRegionTranslationContext::CreateInstructionExitBlock(LLVMRegionTranslationContext::LeaveReasons reason)
{
	// Now create the slightly more complex block used when leaving when executing an instruction
	::llvm::BasicBlock *block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "", region_fn);
	::llvm::IRBuilder<> builder (block);

	exit_instruction_blocks[reason] = block;

	builder.SetInsertPoint(block);
	exit_instruction_phis[reason] = builder.CreatePHI(txln.types.i32, 4, "pc_phi");
	::llvm::Value *exit_pc = builder.CreateAdd(values.virt_page_base, exit_instruction_phis[reason]);
	builder.CreateCall(txln.jit_functions.cpu_write_pc, { values.state_val, values.reg_state_val, exit_pc });

	if (archsim::options::Verbose) {
//		EmitCounterUpdate(builder, txln.twu.GetProcessor().metrics.leave_jit[(int)reason], 1);
	}

	builder.CreateRet(txln.GetConstantInt32((uint32_t)reason));

	return true;
}


bool LLVMTranslationContext::BuildEntryBlock(LLVMRegionTranslationContext& rtc, uint32_t constant_page_base)
{
	assert(!constant_page_base);

	// Build the entry block.
	rtc.builder.SetInsertPoint(rtc.entry_block);

	// First, store the incoming function argument as the state value.
	rtc.values.state_val = &*rtc.region_fn->arg_begin();

	// Now, acquire a pointer to the gensim state.
	::llvm::Value *gensim_state_ptr = nullptr; //rtc.builder.CreateStructGEP(rtc.values.state_val, gensim::CpuStateEntries::CpuState_gensim_state, "gensim_state_ptr");
	AddAliasAnalysisNode((::llvm::Instruction *)gensim_state_ptr, TAG_CPU_STATE);
	rtc.values.reg_state_val = rtc.builder.CreateLoad(gensim_state_ptr, "gensim_state");

	// Also acquire a pointer to the CPU context, i.e. the processor object.
	::llvm::Value *cpu_ctx_ptr = nullptr; //rtc.builder.CreateStructGEP(rtc.values.state_val, gensim::CpuStateEntries::CpuState_cpu_ctx, "cpu_ctx_ptr");
	AddAliasAnalysisNode((::llvm::Instruction *)cpu_ctx_ptr, TAG_CPU_STATE);
	rtc.values.cpu_ctx_val = rtc.builder.CreateLoad(cpu_ctx_ptr, "cpu_ctx");

	::llvm::Value *region_txln_cache_ptr = nullptr; //rtc.builder.CreateStructGEP(rtc.values.state_val, gensim::CpuStateEntries::CpuState_jit_entry_table, "region_txln_cache_ptr_ptr");
	AddAliasAnalysisNode((::llvm::Instruction *)region_txln_cache_ptr, TAG_CPU_STATE);
	rtc.values.region_txln_cache_ptr_val = rtc.builder.CreateLoad(region_txln_cache_ptr, "region_txln_cache");

	// Create temporary variables for reading guest memory.
	rtc.values.mem_read_temp_8 = rtc.builder.CreateAlloca(types.i8, nullptr, "mem_read_temp_8");
	rtc.values.mem_read_temp_16 = rtc.builder.CreateAlloca(types.i16, nullptr, "mem_read_temp_16");
	rtc.values.mem_read_temp_32 = rtc.builder.CreateAlloca(types.i32, nullptr, "mem_read_temp_32");

	::llvm::Value *pc;
	if (constant_page_base != 0) {
		pc = GetConstantInt32(constant_page_base);
	} else {
		pc = rtc.builder.CreateCall(jit_functions.cpu_read_pc, {rtc.values.state_val, rtc.values.reg_state_val}, "pc");
	}

	rtc.values.virt_page_base = rtc.builder.CreateAnd(pc, GetConstantInt32((uint32_t)(~profile::RegionArch::PageMask)), "virt_page_base");
	rtc.values.virt_page_idx = rtc.builder.CreateLShr(pc, GetConstantInt32((uint32_t)(profile::RegionArch::PageBits)), "virt_page_idx");

	// If we're running in verbose mode, emit a translation counter increment
	if (archsim::options::Verbose) {
//		rtc.EmitCounterUpdate(rtc.builder, twu.GetProcessor().metrics.txlns_invoked, 1);
	}

	// Take and check checksum of page under translation
	if(archsim::options::JitChecksumPages) {
//		uint32_t checksum = jitChecksumPage(&twu.GetProcessor(), twu.GetRegion().GetPhysicalBaseAddress());
//		::llvm::Value *computed_checksum = rtc.builder.CreateCall2(jit_functions.jit_checksum_page, rtc.values.cpu_ctx_val, ::llvm::ConstantInt::get(types.i32, twu.GetRegion().GetPhysicalBaseAddress()));
//		rtc.builder.CreateCall2(jit_functions.jit_check_checksum, computed_checksum, ::llvm::ConstantInt::get(types.i32, checksum));
	}

	// Calculate register pointers
	uint32_t reg_bank_count = 0; 
	
	for(auto entry : twu.GetThread()->GetArch().GetRegisterFileDescriptor().GetEntries()) {
		for(int i = 0; i < entry.second.GetEntryCount(); ++i) {
			
			// TODO
			
//			AddAliasAnalysisToRegBankAccess(ptr, ::llvm::ConstantInt::get(types.i32, r), regbank.BankOffset, regbank.RegisterStride, regbank.RegisterCount);
			rtc.RegSlots[std::make_pair(entry.second.GetID(), i)] = nullptr;
		}
	}

	// Branch to the dispatcher.
	rtc.builder.CreateBr(rtc.dispatch_block);

	return true;
}

bool LLVMTranslationContext::BuildChainBlock(LLVMRegionTranslationContext& rtc)
{
	UNIMPLEMENTED;
//	
//	rtc.builder.SetInsertPoint(rtc.chain_block);
//
//	if(archsim::options::JitDisableBranchOpt) {
//		rtc.EmitLeave(LLVMRegionTranslationContext::UnableToChain);
//	} else {
//		::llvm::Value *pc = rtc.builder.CreateCall(jit_functions.cpu_read_pc, {rtc.values.state_val, rtc.values.reg_state_val}, "pc");
//
//		::llvm::Value *phys_pc, *fault;
//		bool success = rtc.txln.twu.GetProcessor().GetMemoryModel().GetTranslationModel().EmitPerformTranslation(rtc, pc, phys_pc, fault);
//		if(!success) return false;
//
//		::llvm::Value *pc_region = rtc.builder.CreateLShr(phys_pc, GetConstantInt32(profile::RegionArch::PageBits), "pc_region");
//
//		::llvm::Value *target_fn_ptr = rtc.builder.CreateInBoundsGEP(rtc.values.region_txln_cache_ptr_val, pc_region, "target_fn_ptr");
//		AddAliasAnalysisNode((::llvm::Instruction *)target_fn_ptr, TAG_REGION_CHAIN_TABLE);
//
//		::llvm::Value *target_fn = rtc.builder.CreateLoad(target_fn_ptr, "target_fn");
//		::llvm::Value *is_invalid = rtc.builder.CreateICmpEQ(rtc.builder.CreatePtrToInt(target_fn, types.i64), GetConstantInt64(0));
//		is_invalid = rtc.builder.CreateOr(is_invalid, rtc.builder.CreateICmpNE(fault, ::llvm::ConstantInt::get(types.i32, 0)));
//
//		::llvm::BasicBlock *do_chain_block = ::llvm::BasicBlock::Create(llvm_ctx, "can_chain", rtc.region_fn);
//
//		rtc.EmitLeave(LLVMRegionTranslationContext::UnableToChain, is_invalid, do_chain_block);
//
//		rtc.builder.SetInsertPoint(do_chain_block);
//
//		::llvm::CallInst *tail_call = rtc.builder.CreateCall(target_fn, rtc.values.state_val);
//		tail_call->setTailCall(true);
//
//		rtc.builder.CreateRet(tail_call);
//	}

	return true;
}

bool LLVMTranslationContext::BuildInterruptHandlerBlock(LLVMRegionTranslationContext& rtc)
{
	rtc.builder.SetInsertPoint(rtc.interrupt_handler_block);

	// Create a call to the CPU to handle the pending actions.
	::llvm::Value *should_exit = rtc.builder.CreateCall(rtc.txln.jit_functions.cpu_handle_pending_action, rtc.values.cpu_ctx_val);
	//rtc.EmitLeave(LLVMRegionTranslationContext::InterruptActionAbort, false);

	// A non-zero return value from cpu_handle_pending_action indicates that we need
	// to exit the JIT.
	should_exit = rtc.builder.CreateICmpNE(should_exit, GetConstantInt32(0));

	// Try and dispatch - this might chain for us.
	rtc.EmitLeave(LLVMRegionTranslationContext::InterruptActionAbort, should_exit, rtc.dispatch_block);

	return true;
}

bool LLVMTranslationContext::PopulateBlockMap(LLVMRegionTranslationContext& rtc)
{
	// Generate an LLVM block for each block in this region, and mark entry blocks
	// as indirect, so that they get added to the jump table.
	for (auto block : twu.GetBlocks()) {
		// Generate a friendly name for this block
		std::stringstream block_name;
		block_name << "guest_block_" << std::hex << block.second->GetOffset();

		// Create the actual LLVM block
		::llvm::BasicBlock *llvm_block = ::llvm::BasicBlock::Create(llvm_ctx, block_name.str(), rtc.region_fn);

		// If the block is an entry block, it MUST be marked as indirect.
		if (block.second->IsEntryBlock())
			AddIndirectTarget(*block.second);

		// Store the LLVM block in the block map.
		llvm_block_map[block.second->GetOffset()] = llvm_block;
	}

	return true;
}

bool LLVMTranslationContext::Translate(Translation*& translation, TranslationTimers& timers)
{
	if(!this->twu.GetRegion().IsValid()) return false;

//	if (archsim::options::Verbose) timers.generation.Start();

	if(archsim::options::JitLoadTranslations) {
		if(LoadTranslation(translation)) {
			LC_DEBUG1(LogTranslate) << "Successfully loaded a txln for " << twu.GetRegion().GetPhysicalBaseAddress();
			return true;
		}
	}

	// Create the IR builder, and initialise the region translation context.
	::llvm::IRBuilder<> builder(llvm_ctx);
	LLVMRegionTranslationContext rtc(*this, builder);

	// Create the LLVM function to represent this region.
	if (!CreateLLVMFunction(rtc))
		return false;

	// Create the default function blocks.
	if (!CreateDefaultBlocks(rtc))
		return false;

	// Build the entry block.
	if (!BuildEntryBlock(rtc, 0))
		return false;

	// Build the chain block.
	if (!BuildChainBlock(rtc))
		return false;

	// Build the interrupt handler block.
	if (!BuildInterruptHandlerBlock(rtc))
		return false;

	// Populate the LLVM block map.
	if (!PopulateBlockMap(rtc))
		return false;

	// Translate each block in this work unit.
	for (auto block : twu.GetBlocks()) {
		::llvm::BasicBlock *llvm_block = llvm_block_map[block.second->GetOffset()];
		LLVMBlockTranslationContext btc(rtc, *block.second, llvm_block);

		if (!btc.Translate())
			return false;
	}

	const uint32_t PageSize = profile::RegionArch::PageSize;
	const uint32_t JumpTableSlots = PageSize >> BLOCK_ALIGN_BITS;

	// Initialise the jump table with default entries.
	for (uint32_t slot = 0; slot < JumpTableSlots; slot++)
		rtc.jump_table_entries[slot] = (::llvm::Constant *)::llvm::BlockAddress::get(rtc.GetExitBlock(LLVMRegionTranslationContext::NoIndirectTarget));

	// Fill in the jump table with block addresses of indirect targets.
	for (auto indirect_target_block : indirect_targets) {
		addr_t block_offset = indirect_target_block->GetOffset();
		assert(block_offset < PageSize);

		uint32_t slot = block_offset >> BLOCK_ALIGN_BITS;
		rtc.jump_table_entries[slot] = ::llvm::BlockAddress::get(llvm_block_map[block_offset]);
	}

	::llvm::ArrayType *jump_table_type = ::llvm::ArrayType::get(types.pi8, JumpTableSlots);
	::llvm::Constant *jump_table_init = ::llvm::ConstantArray::get(jump_table_type, ::llvm::ArrayRef< ::llvm::Constant *>(rtc.jump_table_entries, JumpTableSlots));
	rtc.jump_table = new ::llvm::GlobalVariable(*llvm_module, jump_table_type, true, ::llvm::GlobalValue::InternalLinkage, jump_table_init, "jump_table");

	if (!CreateDispatcher(rtc, rtc.dispatch_block))
		return false;

//	if (archsim::options::Verbose) timers.generation.Stop();

	return Compile(rtc.region_fn, translation, timers);
}

bool LLVMTranslationContext::CreateDispatcher(LLVMRegionTranslationContext& rtc, ::llvm::BasicBlock *dispatcher_block)
{
	::llvm::IRBuilder<> builder(dispatcher_block);

	::llvm::Value *pc = builder.CreateCall(jit_functions.cpu_read_pc, {rtc.values.state_val, rtc.values.reg_state_val}, "pc");
	::llvm::Value *pc_region = builder.CreateLShr(pc, GetConstantInt32(profile::RegionArch::PageBits), "pc_region");
	::llvm::Value *in_this_region = builder.CreateICmpEQ(pc_region, rtc.values.virt_page_idx, "in_this_region");

	::llvm::BasicBlock *do_dispatch_block = ::llvm::BasicBlock::Create(llvm_ctx, "do_dispatch", rtc.region_fn);

	builder.CreateCondBr(in_this_region, do_dispatch_block, rtc.GetExitBlock(LLVMRegionTranslationContext::UnableToChain)); //);

	builder.SetInsertPoint(do_dispatch_block);

	::llvm::Value *pc_offset = builder.CreateAnd(pc, GetConstantInt32(profile::RegionArch::PageMask), "masked_pc");
	pc_offset = builder.CreateLShr(pc_offset, GetConstantInt32(BLOCK_ALIGN_BITS), "jump_table_offset");

	::llvm::Value *idx0 = GetConstantInt32(0);
	::llvm::Value *indicies[] = {idx0, pc_offset};
	::llvm::Value *jump_table_elem = builder.CreateInBoundsGEP(rtc.jump_table, ::llvm::ArrayRef< ::llvm::Value *>(indicies, 2), "jt_elem_ptr");

	AddAliasAnalysisNode((::llvm::Instruction *)jump_table_elem, TAG_JT_ELEMENT);

	::llvm::Value *loaded_address = builder.CreateLoad(jump_table_elem);

	::llvm::IndirectBrInst *branch = builder.CreateIndirectBr(loaded_address, indirect_targets.size() + 1);
	branch->addDestination(rtc.GetExitBlock(LLVMRegionTranslationContext::NoIndirectTarget));

	for (auto indirect_target_block : indirect_targets) {
		auto new_dest = llvm_block_map[indirect_target_block->GetOffset()];
		assert(new_dest);
		branch->addDestination(new_dest);
	}

	return true;
}

::llvm::Value *LLVMTranslationContext::GetConstantInt8(uint8_t v)
{
	return ::llvm::ConstantInt::get(types.i8, (uint64_t)v);
}

::llvm::Value *LLVMTranslationContext::GetConstantInt16(uint16_t v)
{
	return ::llvm::ConstantInt::get(types.i16, (uint64_t)v);
}

::llvm::Value *LLVMTranslationContext::GetConstantInt32(uint32_t v)
{
	return ::llvm::ConstantInt::get(types.i32, (uint64_t)v);
}

::llvm::Value *LLVMTranslationContext::GetConstantInt64(uint64_t v)
{
	return ::llvm::ConstantInt::get(types.i64, (uint64_t)v);
}

::llvm::BasicBlock *LLVMRegionTranslationContext::CreateBlock(std::string name)
{
	return ::llvm::BasicBlock::Create(txln.llvm_ctx, name, region_fn);
}

void LLVMRegionTranslationContext::EmitCounterUpdate(::llvm::IRBuilder<> &builder, archsim::util::Counter64& counter, int64_t increment)
{
	uint64_t *const real_counter_ptr = counter.get_ptr();
	::llvm::Value *counter_ptr = builder.CreateIntToPtr(txln.GetConstantInt64((uint64_t)real_counter_ptr), txln.types.pi64, "counter_ptr");

	if (counter_ptr->getValueID() == ::llvm::Value::ConstantExprVal) {
		counter_ptr = ((::llvm::ConstantExpr *)counter_ptr)->getAsInstruction();
		builder.Insert((::llvm::Instruction *)counter_ptr);
	}

	txln.AddAliasAnalysisNode((::llvm::Instruction *)counter_ptr, TAG_METRIC);
	::llvm::Value *loaded_val = builder.CreateLoad((::llvm::Instruction *)counter_ptr);
	::llvm::Value *added_val = builder.CreateAdd(loaded_val, txln.GetConstantInt64(increment));
	builder.CreateStore(added_val, counter_ptr);
}

void LLVMRegionTranslationContext::EmitCounterPointerUpdate(::llvm::Value *ptr, int64_t increment)
{
	::llvm::Value *counter_ptr = builder.CreateIntToPtr(ptr, txln.types.pi64, "counter_ptr");

	if (counter_ptr->getValueID() == ::llvm::Value::ConstantExprVal) {
		counter_ptr = ((::llvm::ConstantExpr *)counter_ptr)->getAsInstruction();
		builder.Insert((::llvm::Instruction *)counter_ptr);
	}

	txln.AddAliasAnalysisNode((::llvm::Instruction *)counter_ptr, TAG_METRIC);
	::llvm::Value *loaded_val = builder.CreateLoad((::llvm::Instruction *)counter_ptr);
	::llvm::Value *added_val = builder.CreateAdd(loaded_val, txln.GetConstantInt64(increment));
	builder.CreateStore(added_val, counter_ptr);
}

void LLVMRegionTranslationContext::EmitHistogramUpdate(archsim::util::Histogram& histogram, uint32_t idx, int64_t increment)
{
	uint64_t *const real_counter_ptr = histogram.get_value_ptr_at_index(idx);
	::llvm::Value *counter_ptr = builder.CreateIntToPtr(txln.GetConstantInt64((uint64_t)real_counter_ptr), txln.types.pi64, "histo_ptr");

	if (counter_ptr->getValueID() == ::llvm::Value::ConstantExprVal) {
		counter_ptr = ((::llvm::ConstantExpr *)counter_ptr)->getAsInstruction();
		builder.Insert((::llvm::Instruction *)counter_ptr);
	}

	//txln.AddAliasAnalysisNode((::llvm::Instruction *)counter_ptr, TAG_METRIC);
	::llvm::Value *loaded_val = builder.CreateLoad((::llvm::Instruction *)counter_ptr);
	::llvm::Value *added_val = builder.CreateAdd(loaded_val, txln.GetConstantInt64(increment));
	builder.CreateStore(added_val, counter_ptr);
}

::llvm::Value *LLVMRegionTranslationContext::GetSlot(std::string name, ::llvm::Type *type)
{
	if(Slots.count(name)) return Slots.at(name);
	Slots[name] = new ::llvm::AllocaInst(type, 0, &entry_block->getInstList().front());
	return Slots.at(name);
}

void LLVMRegionTranslationContext::EmitPublishEvent(PubSubType::PubSubType type, std::vector<::llvm::Value *> data)
{
//	auto &pubsub = txln.twu.GetThread()->GetEmulationModel().GetSystem().GetPubSub();
//	auto subscribers = pubsub.GetSubscribers(type);
//
//	if(subscribers.empty()) return;
//
//	std::vector<::llvm::Type*> types;
//	for(auto d : data) types.push_back(d->getType());
//	::llvm::StructType *data_type = ::llvm::StructType::get(txln.llvm_ctx, types, true);
//
//	std::ostringstream str;
//	str << "Event" << (uint32_t)type;
//
//	::llvm::Value *slot = GetSlot(str.str(), data_type);
//
//	for(uint32_t i = 0; i < data.size(); ++i) {
//		::llvm::Value *entry_ptr = builder.CreateConstGEP2_32(slot, 0, i);
//		builder.CreateStore(data.at(i), entry_ptr);
//	}
//
//	::llvm::Value *cast_slot = builder.CreateBitCast(slot, txln.types.pi8);
//
//	::llvm::Type *fnty = ::llvm::FunctionType::get(txln.types.vtype, std::vector<::llvm::Type*> {txln.types.i32, txln.types.pi8, txln.types.pi8}, false);
//	fnty = ::llvm::PointerType::get(fnty, 0);
//
//	for(auto subscriber : subscribers) {
//		::llvm::Function *fn = (::llvm::Function*)builder.CreateIntToPtr(txln.GetConstantInt64((uint64_t)subscriber->GetCallback()), fnty);
//		::llvm::Value *context = builder.CreateIntToPtr(txln.GetConstantInt64((uint64_t)subscriber->GetContext()), txln.types.pi8);
//		builder.CreateCall(fn, {txln.GetConstantInt32(type), context, cast_slot});
//	}
	
	UNIMPLEMENTED;

}

void LLVMRegionTranslationContext::EmitLeaveInstruction(LeaveReasons reason, addr_t pc_offset, ::llvm::Value *condition, ::llvm::BasicBlock *cont_block, bool invert)
{
	EmitLeaveInstruction(reason, txln.GetConstantInt32(pc_offset), condition, cont_block, invert);
}

void LLVMRegionTranslationContext::EmitLeaveInstruction(LeaveReasons reason, ::llvm::Value *pc_offset, ::llvm::Value *condition, ::llvm::BasicBlock *cont_block, bool invert)
{
	assert((!condition && !cont_block) || (condition && cont_block));

	// if we should be tracing then we need to end the currently traced instruction,
	// but only if the exit condition is true.
	if(txln.twu.ShouldEmitTracing()) {
		EmitLeaveInstructionTrace(reason, pc_offset, condition, cont_block, invert);
	} else {

		// Otherwise, add an entry to the pc selection phi node and emit
		// the correct type of branch depending on whether there is a
		// condition and whether should be inverted

		::llvm::BasicBlock *exit_block = GetInstructionExitBlock(reason);
		::llvm::PHINode *pn = exit_instruction_phis[reason];
		pn->addIncoming(pc_offset, builder.GetInsertBlock());

		if(condition) {
			if(invert)
				builder.CreateCondBr(condition, cont_block, exit_block);
			else
				builder.CreateCondBr(condition, exit_block, cont_block);
		} else {
			builder.CreateBr(exit_block);
		}
	}
}

void LLVMRegionTranslationContext::EmitLeaveInstructionTrace(LeaveReasons reason, ::llvm::Value *pc_offset, ::llvm::Value *condition, ::llvm::BasicBlock *cont_block, bool invert)
{
	assert((!condition && !cont_block) || (condition && cont_block));

	// create a new block, which will end the currently traced instruction, and then branch to the relevant 'leave' block
	::llvm::BasicBlock *trace_end_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "", region_fn);
	::llvm::IRBuilder<> trace_builder (trace_end_block);
	trace_builder.CreateCall(txln.jit_functions.trace_end_insn, values.cpu_ctx_val);
	trace_builder.CreateBr(GetInstructionExitBlock(reason));
	exit_instruction_phis[reason]->addIncoming(pc_offset, trace_end_block);

	// now, create control flow to branch to the trace end block
	if(condition) {
		if(invert) {
			builder.CreateCondBr(condition, cont_block, trace_end_block);
		} else {
			builder.CreateCondBr(condition, trace_end_block, cont_block);
		}
	} else {
		builder.CreateBr(trace_end_block);
	}
}

void LLVMRegionTranslationContext::EmitTakeException(addr_t pc_offset, ::llvm::Value *arg0, ::llvm::Value *arg1)
{
	EmitTakeException(txln.GetConstantInt32(pc_offset), arg0, arg1);
}

void LLVMRegionTranslationContext::EmitTakeException(::llvm::Value *pc_offset, ::llvm::Value *arg0, ::llvm::Value *arg1)
{
	// save back the PC in case it is used by the exception handler
	builder.CreateCall(txln.jit_functions.cpu_write_pc, {values.state_val, values.reg_state_val, CreateMaterialise(pc_offset)});
	::llvm::Value *exception_action = builder.CreateCall(txln.jit_functions.cpu_take_exception, {values.cpu_ctx_val, arg0, arg1});
	::llvm::BasicBlock *continue_block = ::llvm::BasicBlock::Create(txln.llvm_ctx, "", region_fn);

	::llvm::SwitchInst *exception_switch = builder.CreateSwitch(exception_action, continue_block, 2);
	exception_switch->addCase((::llvm::ConstantInt*)txln.GetConstantInt32(archsim::abi::AbortInstruction), GetExitBlock(ExceptionExit));
	exception_switch->addCase((::llvm::ConstantInt*)txln.GetConstantInt32(archsim::abi::AbortSimulation), GetExitBlock(ExceptionExit));

	builder.SetInsertPoint(continue_block);
}

void LLVMRegionTranslationContext::EmitLeave(LeaveReasons reason, ::llvm::Value *condition, ::llvm::BasicBlock *cont_block, bool invert)
{
	assert((!condition && !cont_block) || (condition && cont_block));

	if(condition) {
		if(invert)
			builder.CreateCondBr(condition, cont_block, GetExitBlock(reason));
		else
			builder.CreateCondBr(condition, GetExitBlock(reason), cont_block);
	} else {
		builder.CreateBr(GetExitBlock(reason));
	}
}

void LLVMRegionTranslationContext::EmitStackMap(uint64_t id)
{
	//builder.CreateCall2(txln.intrinsics.stack_map, txln.GetConstantInt32(id), txln.GetConstantInt32(0));
}

::llvm::Value *LLVMRegionTranslationContext::CreateMaterialise(::llvm::Value* offset)
{
	return builder.CreateAdd(values.virt_page_base, offset);
}

::llvm::Value *LLVMRegionTranslationContext::CreateMaterialise(uint32_t offset)
{
	return CreateMaterialise(txln.GetConstantInt32(offset));
}

bool LLVMBlockTranslationContext::Translate()
{
	region.builder.SetInsertPoint(llvm_block);

	if (tbu.IsInterruptCheck()) {
		if (!EmitInterruptCheck()) {
			return false;
		}
	}

	::llvm::Value *pc = region.txln.GetConstantInt32(tbu.GetOffset() + region.txln.twu.GetRegion().GetPhysicalBaseAddress());
	::llvm::Value *insn_count = region.txln.GetConstantInt32(tbu.GetInstructions().size());
	region.EmitPublishEvent(PubSubType::BlockExecute, { pc, insn_count });

	for (auto insn : tbu.GetInstructions()) {
		assert(insn);
		LLVMInstructionTranslationContext itc(*this, *insn);
		if (!itc.Translate())
			return false;
	}

	auto &last_insn = tbu.GetLastInstruction();

	if (tbu.IsSpanning()) {
		return EmitSpanningControlFlow(last_insn);
	}

	uint32_t jump_target, fallthrough_target;
	bool direct_jump, indirect_jump;
	region.txln.gensim_translate->GetJumpInfo(&last_insn.GetDecode(), last_insn.GetOffset(), indirect_jump, direct_jump, jump_target);

	fallthrough_target = last_insn.GetOffset() + last_insn.GetDecode().Instr_Length;

	if (archsim::options::JitDisableBranchOpt) {
		region.builder.CreateBr(region.dispatch_block);
	} else {
		if (direct_jump) {
			return EmitDirectJump(jump_target, fallthrough_target, last_insn.GetDecode().GetIsPredicated());
		} else if (indirect_jump) {
			return EmitIndirectJump(jump_target, fallthrough_target, last_insn.GetDecode().GetIsPredicated());
		} else {
//			LC_WARNING(LogTranslate) << "Invalid jump info for instruction " << std::hex <<  last_insn.GetOffset() << " " << region.txln.twu.GetThread().GetDisasm()->DisasmInstr(last_insn.GetDecode(), last_insn.GetOffset());
			region.builder.CreateBr(region.dispatch_block);
		}
	}

	return true;
}

bool LLVMBlockTranslationContext::EmitInterruptCheck()
{
	UNIMPLEMENTED;
//	
//	if (archsim::options::Verbose) {
////		region.EmitCounterUpdate(region.builder, region.txln.twu.GetProcessor().metrics.interrupt_checks, 1);
//	}
//
//	std::stringstream block_continue_name;
//	block_continue_name << "guest_block_impl_" << std::hex << tbu.GetOffset();
//
//	::llvm::BasicBlock *block_continue = ::llvm::BasicBlock::Create(region.txln.llvm_ctx, block_continue_name.str(), region.region_fn);
//
//	::llvm::Value *actions_pending = region.builder.CreateStructGEP(region.values.state_val, gensim::CpuStateEntries::CpuState_pending_actions, "actions_pending_ptr");
//	region.txln.AddAliasAnalysisNode((::llvm::Instruction *)actions_pending, TAG_CPU_STATE);
//
//	actions_pending = region.builder.CreateLoad(actions_pending, "actions_pending");
//	actions_pending = region.builder.CreateICmpNE(actions_pending, region.txln.GetConstantInt32(0), "are_actions_pending");
//	region.builder.CreateCondBr(actions_pending, region.interrupt_handler_block, block_continue);
//
//	region.builder.SetInsertPoint(block_continue);

	return true;
}

bool LLVMBlockTranslationContext::EmitDirectJump(virt_addr_t jump_target, virt_addr_t fallthrough_target, bool predicated)
{
	if (predicated) {
		// We need to read the PC at this point to determine where the program is actually going to go.
		::llvm::Value *target_pc = region.builder.CreateCall(region.txln.jit_functions.cpu_read_pc, {region.values.state_val, region.values.reg_state_val}, "target_pc");

		// Now, we must determine if the target PC matches our intended target.
		::llvm::Value *take_jump = region.builder.CreateICmpEQ(target_pc, region.CreateMaterialise(jump_target));

		// Try and look up the target block, and the fall through block.
		::llvm::BasicBlock *target_block = region.txln.GetLLVMBlock(jump_target);
		::llvm::BasicBlock *fallthrough_block = region.txln.GetLLVMBlock(fallthrough_target);

		if (!target_block) {
			// If we can't find the target, then it might be because actually the target lies in another region.  If
			// that is the case, then we should branch to the chaining logic.  Otherwise, we just have to exit.
			if (profile::RegionArch::PageIndexOf(jump_target) != 0)
				target_block = region.chain_block;
			else
				target_block = region.GetExitBlock(LLVMRegionTranslationContext::NoBlockAvailable);
		}

		if (!fallthrough_block) {
			// Similarly, if we can't find the fallthrough, we could be right at the edge of a region and so we
			// can chain into the fallthrough (if the jump is not taken).  Again, otherwise we have to give up.
			if (profile::RegionArch::PageIndexOf(fallthrough_target) != 0)
				fallthrough_block = region.chain_block;
			else
				fallthrough_block = region.GetExitBlock(LLVMRegionTranslationContext::NoBlockAvailable);
		}

		// Now that we finally have two destinations (the taken and not-taken cases) we can emit a branch.  But, if the
		// destinations are the same, we may as well just emit a direct branch there.
		if (target_block == fallthrough_block) {
			region.builder.CreateBr(target_block);
		} else {
			// If we have different destinations, then emit a conditional branch.
			region.builder.CreateCondBr(take_jump, target_block, fallthrough_block);
		}
	} else {
		::llvm::BasicBlock *target = region.txln.GetLLVMBlock(jump_target);
		if (target == nullptr) {
			// We don't know about the jump target, so as a last ditch attempt to do something clever, let's see if
			// we can chain.
			if (profile::RegionArch::PageIndexOf(jump_target) != 0)
				region.builder.CreateBr(region.chain_block);
			else
				region.builder.CreateBr(region.GetExitBlock(LLVMRegionTranslationContext::NoBlockAvailable));
		} else {
			region.builder.CreateBr(target);
		}
	}

	return true;
}

bool LLVMBlockTranslationContext::EmitIndirectJump(virt_addr_t jump_target, virt_addr_t fallthrough_target, bool predicated)
{
	// Add all the successors that we know about into the IBTT.
	for (auto succ : tbu.GetSuccessors()) {
		// But not the fallthrough - because we can directly branch to this.  If it turns
		// out the fallthrough should also be indirect, this will be handled elsewhere.
		if (succ->GetOffset() == fallthrough_target)
			continue;

		region.txln.AddIndirectTarget(*succ);
	}

	if (predicated) {
		// If the jump is predicated, then read the target PC to determine where we're going.
		::llvm::Value *target_pc = region.builder.CreateCall(region.txln.jit_functions.cpu_read_pc, {region.values.state_val, region.values.reg_state_val}, "target_pc");

		// Now, decide if the target PC is actually the fallthrough block.
		::llvm::Value *take_fallthrough = region.builder.CreateICmpEQ(target_pc, region.CreateMaterialise(fallthrough_target));

		// Try and lookup the fallthrough block, but if we can't find it we should exit.
		::llvm::BasicBlock *fallthrough_block = region.txln.GetLLVMBlock(fallthrough_target);
		if (!fallthrough_block) {
			fallthrough_block = region.GetExitBlock(LLVMRegionTranslationContext::NoBlockAvailable);
		}

		// Branch directly to the fallthrough block, if taken - otherwise go via the dispatcher.
		region.builder.CreateCondBr(take_fallthrough, fallthrough_block, region.dispatch_block);
	} else {
		// If the jump is not predicated, jump via the dispatcher.  The dispatcher will handle region
		// chaining if necessary.
		region.builder.CreateBr(region.dispatch_block);
	}

	return true;
}

bool LLVMBlockTranslationContext::EmitSpanningControlFlow(TranslationInstructionUnit& last_insn)
{
	::llvm::Value *new_pc = region.CreateMaterialise(last_insn.GetOffset() + last_insn.GetDecode().Instr_Length);
	region.builder.CreateCall(region.txln.jit_functions.cpu_write_pc, {region.values.state_val, region.values.reg_state_val, new_pc});
	region.builder.CreateBr(region.chain_block);

	return true;
}

bool LLVMInstructionTranslationContext::Translate()
{
	if (archsim::options::Verbose) {
//		block.region.EmitCounterUpdate(block.region.builder, block.region.txln.twu.GetProcessor().metrics.native_inst_count, 1);
	}
	if(archsim::options::Profile) {
//		block.region.EmitHistogramUpdate(block.region.txln.twu.GetProcessor().metrics.opcode_freq_hist, tiu.GetDecode().Instr_Code, 1);
	}
	if(archsim::options::ProfilePcFreq) {
//		block.region.EmitHistogramUpdate(block.region.txln.twu.GetProcessor().metrics.pc_freq_hist, block.region.txln.twu.GetRegion().GetPhysicalBaseAddress() + (tiu.GetOffset()), 1);
	}

//	if(tiu.GetDecode().GetUsesPC() || archsim::options::Verify || block.region.txln.twu.GetProcessor().GetMemoryModel().HasEventHandlers() || block.region.txln.twu.ShouldEmitTracing()) {
	::llvm::Value *new_pc = block.region.CreateMaterialise(tiu.GetOffset());
	block.region.builder.CreateCall(block.region.txln.jit_functions.cpu_write_pc, {block.region.values.state_val, block.region.values.reg_state_val,new_pc});
//	}

	//If we're in verify mode, insert a call to sysVerify
	if(archsim::options::Verify) {
		block.region.builder.CreateCall(block.region.txln.jit_functions.sys_verify, block.region.values.cpu_ctx_val);
	}

	// If we've got memory event handlers, we have to emit a fetch event each time we go to execute an instruction.
//	if(block.region.txln.twu.GetThread()->GetMemoryModel().HasEventHandlers()) {
//		::llvm::Value *new_pc = block.region.CreateMaterialise(tiu.GetOffset());
//		for (auto handler : block.region.txln.twu.GetThread()->GetMemoryModel().GetEventHandlers()) {
//			handler->GetTranslator().EmitEventHandler(*this, *handler, archsim::abi::memory::MemoryModel::MemEventFetch, new_pc, 4);
//		}
//	}

	if(block.region.txln.twu.ShouldEmitTracing()) {
		std::vector< ::llvm::Value *> args;
		::llvm::Value *new_pc = block.region.CreateMaterialise(tiu.GetOffset());

		::llvm::Value *exec_mode = block.region.builder.CreateCall(block.region.txln.jit_functions.cpu_exec_mode, block.region.values.cpu_ctx_val);
		exec_mode = block.region.builder.CreateZExtOrTrunc(exec_mode, block.region.txln.types.i8);

		args.push_back(block.region.values.cpu_ctx_val);								// CPU-CTX
		args.push_back(new_pc);															// PC
		args.push_back(block.region.txln.GetConstantInt32(tiu.GetDecode().GetIR()));	// IR
		args.push_back(block.region.txln.GetConstantInt8(block.tbu.GetISAMode()));		// ISA
		args.push_back(exec_mode);														// IRQ
		args.push_back(block.region.txln.GetConstantInt8(1));							// EXEC

		block.region.builder.CreateCall(block.region.txln.jit_functions.trace_start_insn, args);
	}

	bool success = false;
	if (tiu.GetDecode().GetIsPredicated()) {
		success = TranslatePredicated();
	} else {
		success = TranslateNonPredicated();
	}

	if(block.region.txln.twu.ShouldEmitTracing()) {
		block.region.builder.CreateCall(block.region.txln.jit_functions.trace_end_insn, block.region.values.cpu_ctx_val);
	}

	block.region.EmitPublishEvent(PubSubType::InstructionExecute, {block.region.txln.GetConstantInt32(tiu.GetOffset() + block.region.txln.twu.GetRegion().GetPhysicalBaseAddress()), block.region.txln.GetConstantInt32(tiu.GetDecode().Instr_Code)});

	return success;
}

bool LLVMInstructionTranslationContext::TranslatePredicated()
{
	::llvm::Value *predicate_result, *v;
	block.region.txln.gensim_translate->EmitPredicate(*this, predicate_result, block.region.txln.twu.ShouldEmitTracing());

	std::stringstream exec_name, else_name, post_name;
	exec_name << "insn_exec_" << std::hex << tiu.GetOffset();
	else_name << "insn_else_" << std::hex << tiu.GetOffset();
	post_name << "insn_post_" << std::hex << tiu.GetOffset();

	::llvm::BasicBlock *exec_block = ::llvm::BasicBlock::Create(block.region.txln.llvm_ctx, exec_name.str(), block.region.region_fn);
	::llvm::BasicBlock *post_block = ::llvm::BasicBlock::Create(block.region.txln.llvm_ctx, post_name.str(), block.region.region_fn);
	::llvm::BasicBlock *else_block;

	if (tiu.GetDecode().GetEndOfBlock()) {
		else_block = ::llvm::BasicBlock::Create(block.region.txln.llvm_ctx, else_name.str(), block.region.region_fn);

		::llvm::IRBuilder<> else_block_builder(else_block);
		::llvm::Value *new_pc = block.region.CreateMaterialise(tiu.GetOffset() + tiu.GetDecode().Instr_Length);
		else_block_builder.CreateCall(block.region.txln.jit_functions.cpu_write_pc, {block.region.values.state_val, block.region.values.reg_state_val, new_pc});
		else_block_builder.CreateBr(post_block);
	} else {
		else_block = post_block;
	}

	block.region.builder.CreateCondBr(block.region.builder.CreateCast(::llvm::Instruction::Trunc, predicate_result, block.region.txln.types.i1, "predicate_result"), exec_block, else_block);
	block.region.builder.SetInsertPoint(exec_block);

	block.region.txln.gensim_translate->TranslateInstruction(*this, v, block.region.txln.twu.ShouldEmitTracing());
	block.region.builder.CreateBr(post_block);
	block.region.builder.SetInsertPoint(post_block);

	return true;
}

bool LLVMInstructionTranslationContext::TranslateNonPredicated()
{
	::llvm::Value *v;
	block.region.txln.gensim_translate->TranslateInstruction(*this, v, block.region.txln.twu.ShouldEmitTracing());

	return true;
}

bool LLVMTranslationContext::Compile(::llvm::Function *region_fn, Translation*& translation, TranslationTimers& timers)
{
	if(!this->twu.GetRegion().IsValid()) return false;

	if (archsim::options::Debug) {
		std::stringstream filename;
		filename << "region-" << std::hex << twu.GetRegion().GetPhysicalBaseAddress() << ".ll";

		std::ofstream os_stream(filename.str().c_str(), std::ios_base::out);
		::llvm::raw_os_ostream str(os_stream);

		::llvm::legacy::FunctionPassManager fpm(llvm_module);
		fpm.add(::llvm::createPrintFunctionPass(str, ""));
		fpm.run(*region_fn);
	}

	::llvm::TargetOptions target_opts;
//	target_opts.DisableTailCalls = false;
//	target_opts.PositionIndependentExecutable = false;
	target_opts.EnableFastISel = false;
//	target_opts.NoFramePointerElim = true;
	target_opts.PrintMachineCode = false;

	bool useMemoryManager = true;
	LLVMMemoryManager *memory_manager = nullptr;

	if(useMemoryManager)
		memory_manager = new LLVMMemoryManager(code_pool, code_pool);

#ifdef LLVM_LATEST
	::llvm::ExecutionEngine *engine = ::llvm::EngineBuilder(std::unique_ptr< ::llvm::Module>(llvm_module))
#else
	::llvm::ExecutionEngine *engine = ::llvm::EngineBuilder(std::unique_ptr<::llvm::Module>(llvm_module))
#endif
	                                  .setEngineKind(::llvm::EngineKind::JIT)
	                                  .setOptLevel(::llvm::CodeGenOpt::Aggressive)
	                                  .setRelocationModel(::llvm::Reloc::Static)
	                                  .setCodeModel(::llvm::CodeModel::Large)
	                                  .setTargetOptions(target_opts)
	                                  .create();

	engine->DisableLazyCompilation(true);
	//engine->DisableSymbolSearching(true);

	if (!engine) {
		LC_ERROR(LogTranslate) << "Unable to create LLVM JIT Engine";
		return false;
	}

	//auto time_at_start = std::chrono::high_resolution_clock::now();
	if (!Optimise(engine, region_fn, timers)) {
		delete engine;
		return false;
	}

	if(archsim::options::Verbose) {
		uint64_t ir_count = 0;
		for(const auto &fun : llvm_module->getFunctionList()) {
			for(const auto &block : fun.getBasicBlockList()) {
				ir_count += block.getInstList().size();
			}
		}
//		this->twu.GetProcessor().GetEmulationModel().GetSystem().GetPubSub().Publish(PubSubType::RegionTranslationStatsIRCount, (void*)ir_count);
	}

//	if(archsim::options::JitSaveTranslations) SaveTranslation();

	//auto time_after_opt = std::chrono::high_resolution_clock::now();
	if (archsim::options::Debug) {
		std::stringstream filename;
		filename << "region-" << std::hex << twu.GetRegion().GetPhysicalBaseAddress() << ".opt.ll";

		std::ofstream os_stream(filename.str().c_str(), std::ios_base::out);
		::llvm::raw_os_ostream str(os_stream);

		::llvm::legacy::FunctionPassManager printManager(llvm_module);
#ifdef LLVM_LATEST
		printManager.add(::llvm::createPrintModulePass(str));
#else
		printManager.add(::llvm::createPrintModulePass(str, ""));
#endif
		printManager.run(*region_fn);

		std::stringstream func_filename;
		func_filename << "func-" << std::hex << twu.GetRegion().GetPhysicalBaseAddress() << ".opt.dot";
		DumpFunctionGraph(region_fn, func_filename.str());
	}

//	if (archsim::options::Verbose) timers.compilation.Start();

	if(!this->twu.GetRegion().IsValid()) {
		delete engine;
		return false;
	}

	engine->finalizeObject();

	//auto time_after_compile = std::chrono::high_resolution_clock::now();

//	fprintf(stderr, "XXX %p %lu %lu %lu\n", llvm_module, (time_after_opt-time_at_start).count(), (time_after_compile-time_after_opt).count(), memory_manager->getAllocatedCodeSize());

	translation = new LLVMTranslation((LLVMTranslation::translation_fn)engine->getFunctionAddress(region_fn->getName()), memory_manager);

	if(archsim::options::Debug) {
		std::stringstream str;
		str << "code-" << std::hex << twu.GetRegion().GetPhysicalBaseAddress() << ".bin";
		FILE *f = fopen(str.str().c_str(), "w");
		fwrite((void*)engine->getFunctionAddress(region_fn->getName()),1, translation->GetCodeSize(), f);
		fclose(f);
	}

	region_fn->deleteBody();
	if(useMemoryManager)
		delete engine;

//	if (archsim::options::Verbose) timers.compilation.Stop();

	for (auto block : twu.GetBlocks()) {
		if (block.second->IsEntryBlock()) {
			LC_DEBUG2(LogTranslate) << "Translation for " << std::hex << twu.GetRegion().GetPhysicalBaseAddress() << " contains " << block.second->GetOffset();
			translation->AddContainedBlock(block.second->GetOffset());
		}
	}

	return true;
}

bool LLVMTranslationContext::LoadTranslation(Translation*& translation)
{
//	std::ostringstream mem_str, bc_str;
//	host_addr_t region;
//	mem_str << "txln/txln_" << twu.GetRegion().GetPhysicalBaseAddress() << ".mem";
//	bc_str << "txln/txln_" << twu.GetRegion().GetPhysicalBaseAddress() << ".bc";
//
//	FILE *f = fopen(mem_str.str().c_str(), "r");
//	if(!f) return false;
//	char mem_buffer[4096];
//	fread(mem_buffer, 1, 4096, f);
//	fclose(f);
//	twu.GetProcessor().GetEmulationModel().GetMemoryModel().LockRegion(twu.GetRegion().GetPhysicalBaseAddress(), 4096, region);
//	if(memcmp(mem_buffer, region, 4096)) {
//		LC_DEBUG1(LogTranslate) << "Failed to load a txln for " << twu.GetRegion().GetPhysicalBaseAddress() << ": memory mismatch";
//		return false;
//	}
//
//	LC_DEBUG2(LogTranslate) << "Attempting to load bitcode for txln " << twu.GetRegion().GetPhysicalBaseAddress();
//
//	std::stringstream region_name;
//	region_name << "guest_region_" << std::hex << twu.GetRegion().GetPhysicalBaseAddress();
//
//	auto buffer = ::llvm::MemoryBuffer::getFile(bc_str.str());
//	if(buffer.getError()) {
//		LC_DEBUG1(LogTranslate) << "Failed to load a txln for " << twu.GetRegion().GetPhysicalBaseAddress() << ": membuffer error";
//		return false;
//	}
//
//	auto eo_mod = ::llvm::parseBitcodeFile(buffer.get().get(), llvm_ctx);
//	if(eo_mod.getError()) {
//		LC_DEBUG1(LogTranslate) << "Failed to load a txln for " << twu.GetRegion().GetPhysicalBaseAddress() << ": parse error";
//		return false;
//	}
//	::llvm::Module *mod = eo_mod.get();
//
//	::llvm::TargetOptions target_opts;
//	target_opts.DisableTailCalls = false;
//	target_opts.PositionIndependentExecutable = false;
//	target_opts.EnableFastISel = false;
//	target_opts.NoFramePointerElim = true;
//	target_opts.PrintMachineCode = false;
//
//	bool useMemoryManager = true;
//	LLVMMemoryManager *memory_manager = nullptr;
//
//	if(useMemoryManager)
//		memory_manager = new LLVMMemoryManager(code_pool, code_pool);
//
//#ifdef LLVM_LATEST
//	::llvm::ExecutionEngine *engine = ::llvm::EngineBuilder(std::unique_ptr< ::llvm::Module>(mod))
//#else
//	::llvm::ExecutionEngine *engine = ::llvm::EngineBuilder(mod)
//	                                  .setUseMCJIT(true)
//#endif
//	                                  .setEngineKind(::llvm::EngineKind::JIT)
//	                                  .setOptLevel(::llvm::CodeGenOpt::Aggressive)
//	                                  .setRelocationModel(::llvm::Reloc::Static)
//	                                  .setCodeModel(::llvm::CodeModel::Large)
//	                                  .setMCJITMemoryManager(memory_manager)
//	                                  .setTargetOptions(target_opts)
//	                                  .create();
//
//	engine->DisableLazyCompilation(true);
//	engine->finalizeObject();
//
//	translation = new LLVMTranslation((LLVMTranslation::translation_fn)engine->getFunctionAddress(region_name.str()), memory_manager);
//
//	if(useMemoryManager)
//		delete engine;
//
//	for (auto block : twu.GetBlocks()) {
//		if (block.second->IsEntryBlock()) {
//			LC_DEBUG2(LogTranslate) << "Translation for " << std::hex << twu.GetRegion().GetPhysicalBaseAddress() << " contains " << block.second->GetOffset();
//			translation->AddContainedBlock(block.second->GetOffset());
//		}
//	}
//
//	return true;
	
	UNIMPLEMENTED;
}

void LLVMTranslationContext::SaveTranslation()
{
//	std::ostringstream mem_str, bc_str;
//	host_addr_t region;
//
//	mem_str << "txln/txln_" << twu.GetRegion().GetPhysicalBaseAddress() << ".mem";
//	bc_str << "txln/txln_" << twu.GetRegion().GetPhysicalBaseAddress() << ".bc";
//
//	mkdir("txln", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
//	FILE *f = fopen(mem_str.str().c_str(), "w");
//	twu.GetProcessor().GetEmulationModel().GetMemoryModel().LockRegion(twu.GetRegion().GetPhysicalBaseAddress(), 4096, region);
//	fwrite(region, 1, 4096, f);
//	fclose(f);
//
//	std::ofstream out_file(bc_str.str(), std::ofstream::out);
//	::llvm::raw_os_ostream llvm_ostream(out_file);
//	::llvm::WriteBitcodeToFile(llvm_module, llvm_ostream);
//	llvm_ostream.flush();
//	out_file.close();
	UNIMPLEMENTED;
}

void LLVMTranslationContext::DumpFunctionGraph(::llvm::Function* fn, std::string filename)
{
	FILE *f = fopen(filename.c_str(), "wt");

	fprintf(f, "digraph a {\n");

	for (auto BI = fn->begin(), BE = fn->end(); BI != BE; ++BI) {
		fprintf(f, "\"%s\"\n", BI->getName().str().c_str());

		auto TI = BI->getTerminator();
		for (unsigned i = 0; i < TI->getNumSuccessors(); i++) {
			auto SB = TI->getSuccessor(i);
			fprintf(f, "\"%s\" -> \"%s\"\n", BI->getName().str().c_str(), SB->getName().str().c_str());
		}
	}

	fprintf(f, "}\n");
	fclose(f);
}

bool LLVMTranslationContext::Optimise(::llvm::ExecutionEngine *engine, ::llvm::Function *region_fn, TranslationTimers& timers)
{
//	if (archsim::options::Verbose) timers.optimisation.Start();
	::llvm::legacy::FunctionPassManager internalize(llvm_module);
	
	std::vector<const char *> fns;
	fns.push_back(strdup(region_fn->getName().str().c_str()));

//	internalize.addPass(::llvm::createInternalizePass(fns));
	internalize.run(*region_fn);

	for(auto i : fns) free((void*)i);

	if (archsim::options::JitDebugAA)
		fprintf(stderr, "Starting optimisation");
//	bool result = optimiser.Optimise(llvm_module, &engine->getDataLayout());
	if (archsim::options::JitDebugAA)
		fprintf(stderr, "Completed optimisation");

//	if (archsim::options::Verbose) timers.optimisation.Stop();

//	return result;
	return false;
}

::llvm::Function *LLVMTranslationContext::GetMemReadFunction(uint32_t width)
{
	return nullptr;
//	if(archsim::options::MemoryModel != "contiguous") return false;
//
//	::llvm::Type *ptr_type = nullptr;
//	::llvm::Type *value_type = nullptr;
//	switch(width) {
//		case 8:
//			ptr_type = types.pi8;
//			break;
//		case 16:
//			ptr_type = types.pi16;
//			break;
//		case 32:
//			ptr_type = types.pi32;
//			break;
//		default:
//			assert(false);
//	}
//
//	std::vector<::llvm::Type *> param_types { types.cpu_ctx, types.i32, types.pi32 };
//	::llvm::FunctionType *ftype = ::llvm::FunctionType::get(types.i32, param_types, false);
//
//	std::stringstream fname;
//	fname << "fn_mem_read_" << width;
//
//
//	::llvm::Function *mem_fun = (::llvm::Function*)llvm_module->getFunction(fname.str());
//	if(mem_fun == nullptr) mem_fun = ::llvm::Function::Create(ftype, ::llvm::Function::InternalLinkage, fname.str(), llvm_module);
//	auto param_it = mem_fun->getArgumentList().begin();
//
//	::llvm::Value *param_state = param_it++;
//	::llvm::Value *param_addr = param_it++;
//	::llvm::Value *param_value = param_it++;
//
//	::llvm::BasicBlock *fn_block = ::llvm::BasicBlock::Create(llvm_ctx, "", mem_fun, nullptr);
//
//	// assume a contiguous backing memory
//	abi::memory::ContiguousMemoryTranslationModel *ctm = (abi::memory::ContiguousMemoryTranslationModel*)&mtm;
//
//	::llvm::IRBuilder<> builder (fn_block);
//
//	//get base memory pointer
//	::llvm::Value *mem_base_ptr = ::llvm::ConstantInt::get(types.i64, (uint64_t)ctm->GetContiguousMemoryBase());
//
//	//calculate address
//	::llvm::Value *ptr = builder.CreateAdd(mem_base_ptr, builder.CreateZExt(param_addr, types.i64));
//
//	ptr = builder.CreateIntToPtr(ptr, ptr_type);
//
//	::llvm::Value *value = builder.CreateLoad(ptr, false);
//	value = builder.CreateZExtOrTrunc(value, types.i32);
//	builder.CreateStore(value, param_value);
//
//	builder.CreateRet(::llvm::ConstantInt::get(types.i32, 0));
//
//	return mem_fun;
}

::llvm::Function *LLVMTranslationContext::GetMemWriteFunction(uint32_t width)
{
	return nullptr;
//	if(archsim::options::MemoryModel != "contiguous") return false;
//
//	std::vector<::llvm::Type *> param_types { types.cpu_ctx, types.i32, types.i32 };
//	::llvm::FunctionType *ftype = ::llvm::FunctionType::get(types.i32, param_types, false);
//
//	std::stringstream fname;
//	fname << "fn_mem_write_" << width;
//
//
//	::llvm::Function *mem_fun = (::llvm::Function*)llvm_module->getFunction(fname.str());
//	if(mem_fun == nullptr) mem_fun = ::llvm::Function::Create(ftype, ::llvm::Function::InternalLinkage, fname.str(), llvm_module);
//	auto param_it = mem_fun->getArgumentList().begin();
//
//	::llvm::Value *param_state = param_it++;
//	::llvm::Value *param_addr = param_it++;
//	::llvm::Value *param_value = param_it++;
//
//	::llvm::BasicBlock *fn_block = ::llvm::BasicBlock::Create(llvm_ctx, "", mem_fun, nullptr);
//
//	// assume a contiguous backing memory
//	abi::memory::ContiguousMemoryTranslationModel *ctm = (abi::memory::ContiguousMemoryTranslationModel*)&mtm;
//
//	::llvm::IRBuilder<> builder (fn_block);
//
//	//get base memory pointer
//	::llvm::Value *mem_base_ptr = ::llvm::ConstantInt::get(types.i64, (uint64_t)ctm->GetContiguousMemoryBase());
//
//	//calculate address
//	::llvm::Value *ptr = builder.CreateAdd(mem_base_ptr, builder.CreateZExt(param_addr, types.i64));
//
//	::llvm::Type *ptr_type = nullptr;
//	::llvm::Type *value_type = nullptr;
//	switch(width) {
//		case 8:
//			ptr_type = types.pi8;
//			value_type = types.i8;
//			break;
//		case 16:
//			ptr_type = types.pi16;
//			value_type = types.i16;
//			break;
//		case 32:
//			ptr_type = types.pi32;
//			value_type = types.i32;
//			break;
//		default:
//			assert(false);
//	}
//
//	ptr = builder.CreateIntToPtr(ptr, ptr_type);
//	param_value = builder.CreateZExtOrTrunc(param_value, value_type);
//	builder.CreateStore(param_value, ptr, false);
//	builder.CreateRet(::llvm::ConstantInt::get(types.i32, 0));
//
//	return mem_fun;
}
