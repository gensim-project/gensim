/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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

namespace archsim {
    namespace translate {
        namespace adapt {

            using namespace captive::arch::jit;
            using namespace captive::shared;
            
			class BlockJITValues {
			public:
				BlockJITValues(llvm::Module *module);
				
				llvm::Function *cpuTakeExceptionPtr;
				llvm::Function *blkRead8Ptr;
				llvm::Function *blkRead16Ptr;
				llvm::Function *blkRead32Ptr;
				llvm::Function *genc_adc_flags_ptr;
			};
			
            class BlockJITLoweringContext : public captive::arch::jit::lowering::LoweringContext {
            public:
				// Don't need to give a stack frame size since LLVM will be managing that
				BlockJITLoweringContext(archsim::core::thread::ThreadInstance *thread, ::llvm::Module *module, ::llvm::Function *target_fun) : LoweringContext(), values_(module), thread_(thread), target_fun_(target_fun), target_module_(module) { builder_ = new ::llvm::IRBuilder<>(module->getContext()); }

				virtual bool Prepare(const TranslationContext &ctx);

				bool LowerHeader(const TranslationContext& ctx) override {
						return true;
				}

				bool LowerBlock(const TranslationContext& ctx, captive::shared::IRBlockId block_id, uint32_t block_start) override;

				bool PerformRelocations(const TranslationContext& ctx) {
						// nothing to do
						return true;
				}

				size_t GetEncoderOffset() {
						return 0;
				}

				::llvm::IRBuilder<> &GetBuilder() { return *builder_; }

				::llvm::Value *GetValueFor(const IROperand &operand);
				void SetValueFor(const IROperand &operand, ::llvm::Value *value);
				llvm::Value *GetThreadPtr();
				llvm::Value *GetThreadPtrPtr();
	    
				::llvm::BasicBlock *GetLLVMBlock(IRBlockId block_id);
				archsim::core::thread::ThreadInstance *GetThread() { return thread_; }
				
				::llvm::Value *GetTaggedRegisterPointer(const std::string &tag);
				::llvm::Value *GetRegisterPointer(const archsim::RegisterFileEntryDescriptor &reg, int index);
				::llvm::Value *GetRegisterPointer(int offset, int size);
				::llvm::Value *GetRegfilePointer();
				
				BlockJITValues &GetValues() { return values_; }
				
				::llvm::Type *GetLLVMType(uint32_t bytes);
				::llvm::Type *GetLLVMType(const IROperand &op);
				::llvm::LLVMContext &GetLLVMContext() { return target_module_->getContext(); }
				
            private:
				BlockJITValues values_;
				
				archsim::core::thread::ThreadInstance *thread_;
				
				::llvm::Type *GetPointerIntType();
				::llvm::Value *GetRegPtr(const IROperand &op);
				
				
				std::map<IRBlockId, ::llvm::BasicBlock*> block_ptrs_;
				std::map<IRRegId, ::llvm::Value*> reg_ptrs_;
				
				::llvm::Module *target_module_;
				::llvm::Function *target_fun_;
				::llvm::IRBuilder<> *builder_;
            };


        }
    }
}

#endif /* BLOCKJITADAPTORLOWERINGCONTEXT_H */

