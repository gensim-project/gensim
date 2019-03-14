/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   MemoryTranslationModel.h
 * Author: s0457958
 *
 * Created on 02 December 2013, 13:54
 */

#ifndef MEMORYTRANSLATIONMODEL_H
#define MEMORYTRANSLATIONMODEL_H

namespace llvm
{
	class Value;
	class Type;
}

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{
//			class LLVMInstructionTranslationContext;
//			class LLVMRegionTranslationContext;
//			class LLVMTranslationContext;
		}
	}
	namespace abi
	{
		namespace memory
		{
			class MemoryTranslationModel
			{
			public:
				MemoryTranslationModel();
				virtual ~MemoryTranslationModel();

//				virtual bool PrepareTranslation(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx);
//				virtual bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, ::llvm::Value*& fault, ::llvm::Value *address, ::llvm::Type *destinationType, ::llvm::Value *destination);
//				virtual bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, ::llvm::Value*& fault, ::llvm::Value *address, ::llvm::Value *value);
//
//				virtual bool EmitNonPrivilegedRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, ::llvm::Value*& fault, ::llvm::Value *address, ::llvm::Type *destinationType, ::llvm::Value *destination);
//				virtual bool EmitNonPrivilegedWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, ::llvm::Value*& fault, ::llvm::Value *address, ::llvm::Value *value);
//
//				virtual bool EmitPerformTranslation(archsim::translate::translate_llvm::LLVMRegionTranslationContext& ctx, ::llvm::Value *virt_address, ::llvm::Value *&phys_address, ::llvm::Value *&fault);
			};

			class ContiguousMemoryTranslationModel : public MemoryTranslationModel
			{
			public:
				ContiguousMemoryTranslationModel();
				~ContiguousMemoryTranslationModel();

//				bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, ::llvm::Value*& fault, ::llvm::Value* address, ::llvm::Type* destinationType, ::llvm::Value* destination);
//				bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, ::llvm::Value*& fault, ::llvm::Value* address, ::llvm::Value* value);

				void SetContiguousMemoryBase(void* base)
				{
					contiguousMemoryBase = base;
				};

				void *GetContiguousMemoryBase()
				{
					return contiguousMemoryBase;
				}

			private:
				void* contiguousMemoryBase;
			};
		}
	}
}

#endif /* MEMORYTRANSLATIONMODEL_H */
