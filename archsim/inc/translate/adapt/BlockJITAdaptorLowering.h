/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockJITAdaptorLowering.h
 * Author: harry
 *
 * Created on 14 May 2018, 12:42
 */

#ifndef BLOCKJITADAPTORLOWERING_H
#define BLOCKJITADAPTORLOWERING_H

#include "BlockJITToLLVM.h"
#include "blockjit/block-compiler/lowering/InstructionLowerer.h"

#include <llvm/IR/IRBuilder.h>

namespace archsim {
    namespace translate {
        namespace adapt {

            class BlockJITLoweringContext;
            
            class BlockJITAdaptorLowerer : public captive::arch::jit::lowering::InstructionLowerer {
            public:
                BlockJITLoweringContext &GetContext();
		llvm::IRBuilder<> &GetBuilder();
	
		llvm::Value *GetValueFor(const captive::shared::IROperand &operand);
		void SetValueFor(const captive::shared::IROperand &operand, llvm::Value *value);

            };
            
#define DEFINE_LOWERING(x) class BlockJIT ## x ## Lowering : public BlockJITAdaptorLowerer { public: bool Lower(const captive::shared::IRInstruction*& insn) override; };
            DEFINE_LOWERING(ADD);
            DEFINE_LOWERING(BARRIER);
            
        }
    }
}

#endif /* BLOCKJITADAPTORLOWERING_H */

