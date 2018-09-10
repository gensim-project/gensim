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

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

namespace archsim
{
	namespace translate
	{
		namespace tx_llvm
		{
			class LLVMTranslationContext
			{
			public:
				LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::Module *module, archsim::core::thread::ThreadInstance *thread);

				struct {
					llvm::Type *vtype;
					llvm::IntegerType *i1, *i8, *i16, *i32, *i64, *i128;
					llvm::Type *f32, *f64;
					llvm::Type *i8Ptr, *i64Ptr;
				} Types;

				struct {
					llvm::Function *jit_trap;
					llvm::Function *double_sqrt;
					llvm::Function *float_sqrt;

					llvm::Function *ctpop_i32;
					llvm::Function *bswap_i32;
					llvm::Function *bswap_i64;

					llvm::Function *blkRead8, *blkRead16, *blkRead32, *blkRead64;
					llvm::Function *cpuWrite8, *cpuWrite16, *cpuWrite32, *cpuWrite64;

					llvm::Function *cpuTraceInstruction;

					llvm::Function *cpuTraceMemRead8, *cpuTraceMemRead16, *cpuTraceMemRead32, *cpuTraceMemRead64;
					llvm::Function *cpuTraceMemWrite8, *cpuTraceMemWrite16, *cpuTraceMemWrite32, *cpuTraceMemWrite64;

					llvm::Function *cpuTraceBankedRegisterWrite, *cpuTraceRegisterWrite;
					llvm::Function *cpuTraceBankedRegisterRead, *cpuTraceRegisterRead;

					llvm::Function *cpuTraceInsnEnd;


					llvm::Function *TakeException;

				} Functions;

				struct {
					llvm::Value *state_block_ptr;
					llvm::Value *reg_file_ptr;

				} Values;

				archsim::core::thread::ThreadInstance *GetThread()
				{
					return thread_;
				}
				llvm::Value *GetThreadPtr();
				llvm::Value *GetRegStatePtr();
				llvm::Value *GetStateBlockPointer(const std::string &entry);
				llvm::LLVMContext &LLVMCtx;

				llvm::Value *AllocateRegister(llvm::Type *type, int name);
				void FreeRegister(llvm::Type *t, llvm::Value *v, int name);

				llvm::Value *LoadRegister(int name);
				void StoreRegister(int name, llvm::Value *value);

				void ResetRegisters();

				llvm::Module *Module;

				llvm::IRBuilder<> &GetBuilder()
				{
					ASSERT(builder_ != nullptr);
					return *builder_;
				}
				void SetBuilder(llvm::IRBuilder<> &builder);

				const archsim::ArchDescriptor &GetArch()
				{
					return thread_->GetArch();
				}

				llvm::Value *GetRegPtr(int offset, llvm::Type *type);

				void SetBlock(llvm::BasicBlock *block);
				void ResetLiveRegisters();
			private:


				archsim::core::thread::ThreadInstance *thread_;
				llvm::IRBuilder<> *builder_;

				std::unordered_map<llvm::Type *, std::list<llvm::Value*>> free_registers_;
				std::unordered_map<llvm::Type *, std::list<llvm::Value*>> allocated_registers_;
				std::unordered_map<int, llvm::Value*> live_register_values_;
				std::unordered_map<int, llvm::Value*> live_register_pointers_;

				std::map<std::pair<uint32_t, llvm::Type*>, llvm::Value*> guest_reg_ptrs_;
			};
		}
	}
}

#endif /* LLVMTRANSLATIONCONTEXT_H */

