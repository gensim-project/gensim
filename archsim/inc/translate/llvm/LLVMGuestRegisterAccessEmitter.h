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
		namespace translate_llvm
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

			protected:
				void AddAAAIMetadata(llvm::Value *target, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index);
				llvm::MDNode *GetMetadataFor(const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index);

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

			protected:
				virtual llvm::Value *GetRegisterPointer(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index = nullptr);
			};

			class CachingBasicLLVMGuestRegisterAccessEmitter : public BasicLLVMGuestRegisterAccessEmitter
			{
			public:
				CachingBasicLLVMGuestRegisterAccessEmitter(LLVMTranslationContext &ctx);
				~CachingBasicLLVMGuestRegisterAccessEmitter();

			protected:
				llvm::Value* GetRegisterPointer(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index) override;

			private:
				std::map<std::pair<std::string, int>, llvm::Value*> register_pointer_cache_;
			};

			class GEPLLVMGuestRegisterAccessEmitter : public LLVMGuestRegisterAccessEmitter
			{
			public:
				GEPLLVMGuestRegisterAccessEmitter(LLVMTranslationContext &ctx);
				virtual ~GEPLLVMGuestRegisterAccessEmitter();

				llvm::Value* EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index) override;
				void EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value) override;

			private:
				llvm::Value *GetPointerToReg(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg_view, llvm::Value *index);
				llvm::Value *GetPointerToRegBank(llvm::IRBuilder<> &builder, const archsim::RegisterFileEntryDescriptor &reg_view);

				llvm::Type *GetTypeForRegViewEntry(const archsim::RegisterFileEntryDescriptor& reg_view);
				llvm::Type *GetTypeForRegView(const archsim::RegisterFileEntryDescriptor& reg_view);

				std::map<std::pair<std::string, int>, llvm::Value*> register_pointer_cache_;
			};
		}
	}
}

#endif /* LLVMGUESTREGISTERACCESSEMITTER_H */

