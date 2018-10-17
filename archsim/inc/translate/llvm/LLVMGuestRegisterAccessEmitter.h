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

#include <set>
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

				virtual void Finalize();
				virtual void Reset();

				LLVMTranslationContext &GetCtx()
				{
					return ctx_;
				}

			protected:
				void AddAAAIMetadata(llvm::Value *target, const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index);
				llvm::MDNode *GetMetadataFor(const archsim::RegisterFileEntryDescriptor &reg, llvm::Value *index);

				llvm::Type *GetTypeForRegViewEntry(const archsim::RegisterFileEntryDescriptor& reg_view);
				llvm::Type *GetTypeForRegView(const archsim::RegisterFileEntryDescriptor& reg_view);

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

				std::map<std::pair<std::string, int>, llvm::Value*> register_pointer_cache_;
			};

			class ShadowLLVMGuestRegisterAccessEmitter : public LLVMGuestRegisterAccessEmitter
			{
			public:
				ShadowLLVMGuestRegisterAccessEmitter(LLVMTranslationContext &ctx);
				virtual ~ShadowLLVMGuestRegisterAccessEmitter();

				llvm::Value* EmitRegisterRead(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index) override;
				void EmitRegisterWrite(llvm::IRBuilder<>& builder, const archsim::RegisterFileEntryDescriptor& reg, llvm::Value* index, llvm::Value* value) override;
				void Finalize() override;
				void Reset() override;

				using register_descriptor_t = std::pair<const archsim::RegisterFileEntryDescriptor*, llvm::Value*>;
				using interval_t = std::pair<int, int>;
				using intervals_t = std::set<std::pair<int, int>>;
				using allocas_t = std::map<std::pair<int, int>, llvm::AllocaInst*>;

			private:
				llvm::Value *GetUndefPtrFor(const archsim::RegisterFileEntryDescriptor& reg);

				std::map<register_descriptor_t, std::vector<llvm::LoadInst*>> loads_;
				std::map<register_descriptor_t, std::vector<llvm::StoreInst*>> stores_;

				std::pair<int, int> GetInterval(const register_descriptor_t &reg);

				// Figure out which chunks of the register file we should be
				// dealing with.
				intervals_t GetIntervals();

				// Allocate space for the shadow register file
				allocas_t CreateAllocas(const intervals_t &intervals);

				// Fix loads to point to the correct interval instead of undef
				void FixLoads(const allocas_t &allocas);

				// Fix stores to point to the correct interval instead of undef
				void FixStores(const allocas_t &allocas);

				void LoadShadowRegs(llvm::Instruction *insertbefore, const allocas_t &allocas);
				void SaveShadowRegs(llvm::Instruction *insertbefore, const allocas_t &allocas);

				llvm::Value *GetRealRegPtr(llvm::IRBuilder<> &builder, const interval_t &interval);
				llvm::Value *GetShadowRegPtr(llvm::IRBuilder<> &builder, const interval_t &interval, const allocas_t &allocas);
				llvm::Value *GetShadowRegPtr(llvm::IRBuilder<> &builder, const register_descriptor_t &reg, const allocas_t &allocas);
				llvm::Type *GetIntervalType(const interval_t &interval);

			};
		}
	}
}

#endif /* LLVMGUESTREGISTERACCESSEMITTER_H */

