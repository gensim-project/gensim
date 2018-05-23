/*
 * File:   SystemMemoryTranslationModel.h
 * Author: s0457958
 *
 * Created on 11 July 2014, 15:04
 */

#ifndef SYSTEMMEMORYTRANSLATIONMODEL_H
#define	SYSTEMMEMORYTRANSLATIONMODEL_H

#if CONFIG_LLVM

#include "abi/memory/MemoryTranslationModel.h"
#include "translate/llvm/LLVMTranslationContext.h"
#include "gensim/gensim_processor_state.h"

#include "util/RawZoneMap.h"
#include "util/PubSubSync.h"

namespace gensim
{
	class Processor;
}
namespace llvm
{
	class Value;
	class StructType;
}

namespace archsim
{
	namespace abi
	{
		namespace memory
		{

			class SystemMemoryModel;

			class SystemMemoryTranslationModel : public MemoryTranslationModel
			{
			public:
				virtual bool Initialise(SystemMemoryModel &mem_model) = 0;

				virtual bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination) override = 0;
				virtual bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value) override = 0;

				virtual void Flush() = 0;
				virtual void Evict(virt_addr_t virt_addr) = 0;
			};

			class BaseSystemMemoryTranslationModel : public SystemMemoryTranslationModel
			{
			public:
				BaseSystemMemoryTranslationModel();
				~BaseSystemMemoryTranslationModel();

				virtual bool Initialise(SystemMemoryModel &mem_model) override;

				bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination) override;
				bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value) override;

				bool EmitPerformTranslation(archsim::translate::translate_llvm::LLVMRegionTranslationContext& insn_ctx, llvm::Value *virt_address, llvm::Value *&phys_address, llvm::Value *&fault) override;

				void Flush() override;
				void Evict(virt_addr_t virt_addr) override;
			};


			class CacheBasedSystemMemoryTranslationModel : public SystemMemoryTranslationModel
			{
			public:
				CacheBasedSystemMemoryTranslationModel();
				~CacheBasedSystemMemoryTranslationModel();

				virtual bool Initialise(SystemMemoryModel &mem_model) override;

				bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination) override;
				bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value) override;

				void Flush() override;
				void Evict(virt_addr_t virt_addr) override;

			private:
				::llvm::Value *GetCpuInKernelMode(::llvm::IRBuilder<> builder, ::llvm::Value *cpu_state);

				::llvm::Value *GetOrCreateMemReadFn(archsim::translate::translate_llvm::LLVMTranslationContext &llvm_ctx, int width, bool sx);
				::llvm::Value *GetOrCreateMemWriteFn(archsim::translate::translate_llvm::LLVMTranslationContext &llvm_ctx, int width);

			};


			class DirectMappedSystemMemoryTranslationModel : public SystemMemoryTranslationModel
			{
			public:
				virtual bool Initialise(SystemMemoryModel &mem_model) override;

				bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination) override;
				bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value) override;

				void Flush() override;
				void Evict(virt_addr_t virt_addr) override;

			private:
				SystemMemoryModel *mem_model;
				util::PubSubscriber *subscriber;
			};

		}
	}
}

#endif

#endif	/* SYSTEMMEMORYTRANSLATIONMODEL_H */
