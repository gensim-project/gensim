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

#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "BlockJITAdaptorLowering.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

namespace archsim {
    namespace translate {
        namespace adapt {

            using namespace captive::arch::jit;
            using namespace captive::shared;
            
            class BlockJITLoweringContext : public captive::arch::jit::lowering::LoweringContext {
            public:
				// Don't need to give a stack frame size since LLVM will be managing that
				BlockJITLoweringContext(llvm::Module *module, llvm::Function *target_fun) : LoweringContext(), target_fun_(target_fun), target_module_(module) {}

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

				llvm::IRBuilder<> &GetBuilder() { return *builder_; }

				llvm::Value *GetValueFor(const IROperand &operand);
				void SetValueFor(const IROperand &operand, llvm::Value *value);
	    
            private:
				llvm::Module *target_module_;
				llvm::Function *target_fun_;
				llvm::IRBuilder<> *builder_;
            };


        }
    }
}

#endif /* BLOCKJITADAPTORLOWERINGCONTEXT_H */

