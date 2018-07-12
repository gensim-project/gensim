/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SparseMemoryModel.h
 * Author: s0457958
 *
 * Created on 10 December 2013, 09:05
 */

#ifndef SPARSEMEMORYMODEL_H
#define SPARSEMEMORYMODEL_H

#include "abi/memory/MemoryModel.h"
#include "abi/memory/MemoryTranslationModel.h"
#include <map>
#include <string>

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class SparseMemoryModel;

			class SparseMemoryTranslationModel : public MemoryTranslationModel
			{
			public:
				SparseMemoryTranslationModel(SparseMemoryModel& model);
				~SparseMemoryTranslationModel();

				bool PrepareTranslation(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx);
				bool EmitMemoryRead(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, bool sx, llvm::Value*& fault, llvm::Value* address, llvm::Type* destinationType, llvm::Value* destination);
				bool EmitMemoryWrite(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& insn_ctx, int width, llvm::Value*& fault, llvm::Value* address, llvm::Value* value);

			};

			class SparseMemoryModel : public RegionBasedMemoryModel
			{
				friend class SparseMemoryTranslationModel;

			public:
				SparseMemoryModel();
				~SparseMemoryModel();

				bool Initialise() override;
				void Destroy() override;

				virtual uint32_t Read(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Fetch(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Write(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Peek(guest_addr_t addr, uint8_t *data, int size) override;
				virtual uint32_t Poke(guest_addr_t addr, uint8_t *data, int size) override;

				MemoryTranslationModel& GetTranslationModel();
				bool SynchroniseVMAProtection(GuestVMA& vma) override;


			protected:
				bool AllocateVMA(GuestVMA &vma);
				bool DeallocateVMA(GuestVMA &vma);
				bool ResizeVMA(GuestVMA &vma, guest_size_t new_size);
			private:
				char *GetPage(Address addr);

				SparseMemoryTranslationModel* translation_model;
				std::map<Address, char*> data_;
			};
		}
	}
}

#endif /* SPARSEMEMORYMODEL_H */
