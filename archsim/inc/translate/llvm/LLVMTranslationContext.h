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
				LLVMTranslationContext(llvm::LLVMContext &ctx, llvm::IRBuilder<> &builder, archsim::core::thread::ThreadInstance *thread);

				struct {
					llvm::Type *vtype;
					llvm::IntegerType *i1, *i8, *i16, *i32, *i64, *i128;
					llvm::Type *f32, *f64;
					llvm::Type *i8Ptr;
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

				llvm::Value *GetThreadPtr();
				llvm::LLVMContext &LLVMCtx;

				llvm::Value *AllocateRegister(llvm::Type *type);
				void FreeRegister(int width_in_bytes, llvm::Value *v);

				llvm::IRBuilder<> &Builder;
				llvm::Module *Module;

				const archsim::ArchDescriptor &GetArch()
				{
					return thread_->GetArch();
				}
			private:
				archsim::core::thread::ThreadInstance *thread_;

				std::unordered_map<llvm::Type *, std::list<llvm::Value*>> free_registers_;
			};
		}
	}
}

#endif /* LLVMTRANSLATIONCONTEXT_H */

