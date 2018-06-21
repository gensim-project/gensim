/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   BlockJITAdaptorLoweringContext.h
 * Author: harry
 *
 * Created on 14 May 2018, 12:44
 */

#ifndef BLOCKJITADAPTORLOWERINGCONTEXT_H
#define BLOCKJITADAPTORLOWERINGCONTEXT_H

#include "core/arch/RegisterFileDescriptor.h"
#include "blockjit/block-compiler/lowering/LoweringContext.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include <unordered_map>

namespace archsim
{
	namespace translate
	{
		namespace adapt
		{

			using namespace captive::arch::jit;
			using namespace captive::shared;

			class BlockJITValues
			{
			public:
				BlockJITValues(llvm::Module *module);

				llvm::Function *cpuTakeExceptionPtr;
				llvm::Function *cpuReadDevicePtr;
				llvm::Function *cpuWriteDevicePtr;
				llvm::Function *cpuSetFeaturePtr;

				llvm::Function *cpuSetRoundingModePtr;
				llvm::Function *cpuGetRoundingModePtr;
				llvm::Function *cpuSetFlushModePtr;
				llvm::Function *cpuGetFlushModePtr;

				llvm::Function *blkRead8Ptr;
				llvm::Function *blkRead16Ptr;
				llvm::Function *blkRead32Ptr;

				llvm::Function *blkWrite8Ptr;
				llvm::Function *blkWrite16Ptr;
				llvm::Function *blkWrite32Ptr;

				llvm::Function *genc_adc_flags_ptr;
			};

			class BlockJITLoweringContext : public captive::arch::jit::lowering::LoweringContext
			{
			public:
				// Don't need to give a stack frame size since LLVM will be managing that
				BlockJITLoweringContext(const archsim::ArchDescriptor &arch, const archsim::StateBlockDescriptor &sbd, ::llvm::Module *module, ::llvm::Function *target_fun) : LoweringContext(arch, sbd), values_(module), target_fun_(target_fun), target_module_(module)
				{
					builder_ = new ::llvm::IRBuilder<>(module->getContext());
				}
				~BlockJITLoweringContext()
				{
					if(builder_!= nullptr) {
						delete builder_;
					}
				}

				virtual bool Prepare(const TranslationContext &ctx);

				bool LowerHeader(const TranslationContext& ctx) override
				{
					return true;
				}

				bool LowerBlock(const TranslationContext& ctx, captive::shared::IRBlockId block_id, uint32_t block_start) override;

				bool PerformRelocations(const TranslationContext& ctx)
				{
					// nothing to do
					return true;
				}

				size_t GetEncoderOffset()
				{
					return 0;
				}

				::llvm::IRBuilder<> &GetBuilder()
				{
					return *builder_;
				}

				::llvm::Value *GetValueFor(const IROperand &operand);
				void SetValueFor(const IROperand &operand, ::llvm::Value *value);
				llvm::Value *GetStateBlockEntryPtr(const std::string& entry, llvm::Type *type);

				llvm::Value *GetThreadPtr();
				llvm::Value *GetThreadPtrPtr();

				::llvm::BasicBlock *GetLLVMBlock(IRBlockId block_id);

				::llvm::Value *GetTaggedRegisterPointer(const std::string &tag);
				::llvm::Value *GetRegisterPointer(const archsim::RegisterFileEntryDescriptor &reg, int index);
				::llvm::Value *GetRegisterPointer(int offset, int size);
				::llvm::Value *GetRegisterPointer(llvm::Value*, int size);
				::llvm::Value *GetRegfilePointer();
				::llvm::Value *GetStateBlockPtr();

				::llvm::MDNode *GetRegAccessMetadata(int offset, int size);
				::llvm::MDNode *GetRegAccessMetadata();

				BlockJITValues &GetValues()
				{
					return values_;
				}

				::llvm::Type *GetLLVMType(uint32_t bytes);
				::llvm::Type *GetLLVMType(const IROperand &op);
				::llvm::LLVMContext &GetLLVMContext()
				{
					return target_module_->getContext();
				}
				::llvm::Module *GetModule()
				{
					return target_module_;
				}

			private:
				BlockJITValues values_;

				::llvm::Type *GetPointerIntType();
				::llvm::Value *GetRegPtr(const IROperand &op);

				std::unordered_map<IRBlockId, ::llvm::BasicBlock*> block_ptrs_;
				std::unordered_map<IRRegId, ::llvm::Value*> reg_ptrs_;
				std::unordered_map<IRRegId, ::llvm::Value*> cached_reg_ptrs_;
				std::unordered_map<IRRegId, ::llvm::StoreInst*> cached_reg_stores_;
				std::unordered_map<IRRegId, ::llvm::BasicBlock*> cached_reg_blocks_;
				std::map<std::pair<uint32_t, uint32_t>, ::llvm::Value*> greg_ptrs_;

				::llvm::Module *target_module_;
				::llvm::Function *target_fun_;
				::llvm::IRBuilder<> *builder_;
			};


		}
	}
}

#endif /* BLOCKJITADAPTORLOWERINGCONTEXT_H */

