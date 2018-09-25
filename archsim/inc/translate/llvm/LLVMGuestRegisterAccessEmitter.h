/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMGuestRegisterAccessEmitter.h
 * Author: harry
 *
 * Created on 25 September 2018, 13:23
 */

#ifndef LLVMGUESTREGISTERACCESSEMITTER_H
#define LLVMGUESTREGISTERACCESSEMITTER_H

#include "core/arch/RegisterFileDescriptor.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>

namespace archsim
{
	namespace translate
	{
		namespace tx_llvm
		{
			class LLVMTranslationContext;

			class LLVMGuestRegisterAccessEmitter
			{
			public:
				LLVMGuestRegisterAccessEmitter(LLVMTranslationContext &ctx);
				virtual ~LLVMGuestRegisterAccessEmitter();

				virtual void EmitRegisterWrite(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index, llvm::Value *value) = 0;
				virtual llvm::Value *EmitRegisterRead(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index) = 0;

				LLVMTranslationContext &GetCtx()
				{
					return ctx_;
				}

			private:
				LLVMTranslationContext &ctx_;
			};

			class BasicLLVMGuestRegisterAccessEmitter : public LLVMGuestRegisterAccessEmitter
			{
			public:
				BasicLLVMGuestRegisterAccessEmitter(LLVMTranslationContext &ctx);
				virtual ~BasicLLVMGuestRegisterAccessEmitter();

				llvm::Value* EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index) override;
				void EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value) override;

			private:
				llvm::Value *GetRegisterPointer(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index = nullptr);
			};

			class GEPLLVMGuestRegisterAccessEmitter : public LLVMGuestRegisterAccessEmitter
			{
			public:
				GEPLLVMGuestRegisterAccessEmitter(LLVMTranslationContext &ctx);
				virtual ~GEPLLVMGuestRegisterAccessEmitter();

				llvm::Value* EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index) override;
				void EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value) override;

			private:
				llvm::Type *GetTypeForRegViewEntry(const archsim::RegisterFileEntryDescriptor& reg_view);
				llvm::Type *GetTypeForRegView(const archsim::RegisterFileEntryDescriptor& reg_view);
			};
		}
	}
}

#endif /* LLVMGUESTREGISTERACCESSEMITTER_H */

