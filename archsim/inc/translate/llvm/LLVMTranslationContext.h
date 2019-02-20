/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMTranslationContext.h
 * Author: harry
 *
 * Created on 28 August 2018, 16:33
 */

#ifndef LLVMTRANSLATIONCONTEXT_H
#define LLVMTRANSLATIONCONTEXT_H

#include "core/thread/ThreadInstance.h"
#include "gensim/gensim_translate.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/llvm/LLVMGuestRegisterAccessEmitter.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

static void txln_shunt_flush_itlb_entry(void *v, uint64_t addr)
{
	UNIMPLEMENTED;
}
static void txln_shunt_flush_dtlb_entry(void *v, uint64_t addr)
{
	UNIMPLEMENTED;
}

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMTranslationContext
			{
			public:
				LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::Function *fn, archsim::core::thread::ThreadInstance *thread);

				struct {
					llvm::Type *vtype;
					llvm::IntegerType *i1, *i8, *i16, *i32, *i64, *i128;
					llvm::Type *f32, *f64;
					llvm::Type *i8Ptr, *i32Ptr, *i64Ptr;
				} Types;

				struct {
					llvm::Function *jit_trap;
					llvm::Function *double_sqrt;
					llvm::Function *float_sqrt;

					llvm::Function *clz_i32;
					llvm::Function *ctz_i32;
					llvm::Function *clz_i64;
					llvm::Function *ctz_i64;
					llvm::Function *ctpop_i32;
					llvm::Function *bswap_i32;
					llvm::Function *bswap_i64;

					llvm::Function *uadd_with_overflow_8, *uadd_with_overflow_16, *uadd_with_overflow_32, *uadd_with_overflow_64;
					llvm::Function *usub_with_overflow_8, *usub_with_overflow_16, *usub_with_overflow_32, *usub_with_overflow_64;

					llvm::Function *sadd_with_overflow_8, *sadd_with_overflow_16, *sadd_with_overflow_32, *sadd_with_overflow_64;
					llvm::Function *ssub_with_overflow_8, *ssub_with_overflow_16, *ssub_with_overflow_32, *ssub_with_overflow_64;

					llvm::Function *assume;
					llvm::Function *debug_trap;

					llvm::Function *blkRead8, *blkRead16, *blkRead32, *blkRead64;
					llvm::Function *cpuWrite8, *cpuWrite16, *cpuWrite32, *cpuWrite64;

					llvm::Function *cpuEnterUser, *cpuEnterKernel, *cpuPendIRQ, *cpuPushInterrupt;

					llvm::Function *cpuTraceInstruction;

					llvm::Function *cpuTraceMemRead8, *cpuTraceMemRead16, *cpuTraceMemRead32, *cpuTraceMemRead64;
					llvm::Function *cpuTraceMemWrite8, *cpuTraceMemWrite16, *cpuTraceMemWrite32, *cpuTraceMemWrite64;

					llvm::Function *cpuTraceBankedRegisterWrite, *cpuTraceRegisterWrite;
					llvm::Function *cpuTraceBankedRegisterRead, *cpuTraceRegisterRead;

					llvm::Function *cpuTraceInsnEnd;


					llvm::Function *TakeException;

					llvm::Function *dev_read_device, *dev_read_device64;
					llvm::Function *dev_write_device, *dev_write_device64;

					llvm::Function *InstructionTick;

				} Functions;

				struct {
					llvm::Value *state_block_ptr;
					llvm::Value *reg_file_ptr;

					llvm::Value *contiguous_mem_base;

				} Values;

				archsim::core::thread::ThreadInstance *GetThread()
				{
					return thread_;
				}
				llvm::Value *GetThreadPtr(llvm::IRBuilder<> &builder);
				llvm::Value *GetRegStatePtr();
				llvm::Function *GetFunction();
				llvm::Value *GetStateBlockPointer(llvm::IRBuilder<> &builder, const std::string &entry);
				llvm::Value *GetStateBlockPointer();
				llvm::LLVMContext &LLVMCtx;

				llvm::Value *GetTraceStackSlot(llvm::Type *type);

				llvm::Value *AllocateRegister(llvm::Type *type, int name);
				void FreeRegister(llvm::Type *t, llvm::Value *v, int name);

				llvm::Value *LoadGuestRegister(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index);
				void StoreGuestRegister(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index, llvm::Value *value);

				llvm::Value *LoadRegister(llvm::IRBuilder<> &builder, int name);
				void StoreRegister(llvm::IRBuilder<> &builder, int name, llvm::Value *value);

				void ResetRegisters();

				llvm::Module *Module;

				const archsim::ArchDescriptor &GetArch()
				{
					return thread_->GetArch();
				}

				llvm::Value *GetRegPtr(llvm::IRBuilder<> &builder, int offset, llvm::Type *type);
				llvm::Value *GetRegPtr(llvm::IRBuilder<> &builder, llvm::Value *offset, llvm::Type *type);

				void ResetLiveRegisters(llvm::IRBuilder<> &builder);

				void Finalize()
				{
					guest_reg_emitter_->Finalize();
				}
			private:
				std::map<llvm::Type*, llvm::Value *> trace_slots_;

				archsim::core::thread::ThreadInstance *thread_;

				LLVMGuestRegisterAccessEmitter *guest_reg_emitter_;

				llvm::Function *function_;

				std::unordered_map<llvm::Type *, std::list<llvm::Value*>> free_registers_;
				std::unordered_map<llvm::Type *, std::list<llvm::Value*>> allocated_registers_;
				std::unordered_map<int, llvm::Value*> live_register_values_;
				std::unordered_map<int, llvm::Value*> live_register_pointers_;

				std::map<std::pair<uint32_t, llvm::Type*>, llvm::Value*> guest_reg_ptrs_;

				llvm::BasicBlock *prev_reg_block_;
				std::map<std::pair<uint32_t, llvm::Type*>, llvm::Value*> guest_reg_values_;
				std::map<std::pair<uint32_t, llvm::Type*>, llvm::StoreInst*> guest_reg_stores_;
			};

			class LLVMRegionTranslationContext
			{
			public:

				enum ExitReasons {
					EXIT_REASON_NEXTPAGE,
					EXIT_REASON_NOBLOCK,
					EXIT_REASON_PAGECHANGE,
					EXIT_REASON_MESSAGE
				};

				using BlockMap = std::map<archsim::Address, llvm::BasicBlock*>;

				LLVMRegionTranslationContext(LLVMTranslationContext &ctx, llvm::Function *fn, translate::TranslationWorkUnit &work_unit, llvm::BasicBlock *entry_block, gensim::BaseLLVMTranslate *txlt);

				BlockMap &GetBlockMap()
				{
					return block_map_;
				}
				llvm::BasicBlock *GetExitBlock(int exit_reason);

				llvm::BasicBlock *GetEntryBlock()
				{
					return entry_block_;
				}
				llvm::BasicBlock *GetDispatchBlock()
				{
					return dispatch_block_;
				}
				llvm::Function *GetFunction()
				{
					return function_;
				}
				LLVMTranslationContext &GetContext()
				{
					return ctx_;
				}
				gensim::BaseLLVMTranslate *GetTxlt()
				{
					return txlt_;
				}

			private:
				LLVMTranslationContext &ctx_;

				llvm::BasicBlock *entry_block_, *dispatch_block_, *region_chain_block_;
				llvm::Function *function_;

				std::map<int, llvm::BasicBlock*> exit_blocks_;
				BlockMap block_map_;

				gensim::BaseLLVMTranslate *txlt_;

				void BuildDispatchBlock(TranslationWorkUnit &work_unit);
				void BuildSwitchStatement(TranslationWorkUnit &work_unit);
				void BuildJumpTable(TranslationWorkUnit &work_unit);

				llvm::BasicBlock *GetRegionChainBlock();
			};
		}
	}
}

#endif /* LLVMTRANSLATIONCONTEXT_H */

