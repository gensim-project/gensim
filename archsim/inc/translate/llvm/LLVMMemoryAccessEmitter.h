/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

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

			class LLVMMemoryAccessEmitter
			{
			public:
				LLVMMemoryAccessEmitter(LLVMTranslationContext &ctx);
				virtual ~LLVMMemoryAccessEmitter();

				virtual llvm::Value *EmitMemoryRead(llvm::IRBuilder<> &builder, llvm::Value *address, int size, int interface, int mode) = 0;
				virtual void EmitMemoryWrite(llvm::IRBuilder<> &builder, llvm::Value *address, llvm::Value* value, int size, int interface, int mode) = 0;

				virtual void Finalize();
				virtual void Reset();

				LLVMTranslationContext &GetCtx()
				{
					return ctx_;
				}

			private:
				LLVMTranslationContext &ctx_;
			};

			class BaseLLVMMemoryAccessEmitter : public LLVMMemoryAccessEmitter
			{
			public:
				BaseLLVMMemoryAccessEmitter(LLVMTranslationContext &ctx);

				llvm::Value* EmitMemoryRead(llvm::IRBuilder<>& builder, llvm::Value* address, int size, int interface, int mode) override;
				void EmitMemoryWrite(llvm::IRBuilder<>& builder, llvm::Value* address, llvm::Value* value, int size, int interface, int mode) override;
			};

			class CacheLLVMMemoryAccessEmitter : public LLVMMemoryAccessEmitter
			{
			public:
				CacheLLVMMemoryAccessEmitter(LLVMTranslationContext &ctx);

				llvm::Value* EmitMemoryRead(llvm::IRBuilder<>& builder, llvm::Value* address, int size, int interface, int mode) override;
				void EmitMemoryWrite(llvm::IRBuilder<>& builder, llvm::Value* address, llvm::Value* value, int size, int interface, int mode) override;

				void Reset() override;

			private:
				llvm::Value *GetCache(llvm::IRBuilder<> &builder, const std::string &cache_name);
				llvm::Value *GetCacheEntry(llvm::IRBuilder<> &builder, llvm::Value *cache, llvm::Value *address);
				llvm::Value *GetCacheEntry_VirtualTag(llvm::IRBuilder<> &builder, llvm::Value *entry);
				llvm::Value *GetCacheEntry_Data(llvm::IRBuilder<> &builder, llvm::Value *entry);

				llvm::StructType *GetCacheEntryType();
				llvm::Type *GetCacheType();
			};

		}
	}
}
