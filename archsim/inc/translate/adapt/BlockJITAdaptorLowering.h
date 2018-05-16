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
#include "blockjit/IRInstruction.h"

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

				llvm::Value *GetThreadPtr();
				
            };
			
			class BlockJITCMPLowering : public BlockJITAdaptorLowerer {
			public:
				BlockJITCMPLowering(captive::shared::IRInstruction::IRInstructionType type) : type_(type) {}
				captive::shared::IRInstruction::IRInstructionType GetCmpType() { return type_; }
				
				bool Lower(const captive::shared::IRInstruction*& insn) override;
			private:
				captive::shared::IRInstruction::IRInstructionType type_;

			};
            
#define DEFINE_LOWERING(x) class BlockJIT ## x ## Lowering : public BlockJITAdaptorLowerer { public: bool Lower(const captive::shared::IRInstruction*& insn) override; };
            DEFINE_LOWERING(ADCFLAGS);
            DEFINE_LOWERING(ADD);
            DEFINE_LOWERING(AND);
            DEFINE_LOWERING(BARRIER);
            DEFINE_LOWERING(BRANCH);
            DEFINE_LOWERING(CALL);
            DEFINE_LOWERING(COUNT);
            DEFINE_LOWERING(EXCEPTION);
            DEFINE_LOWERING(INCPC);
            DEFINE_LOWERING(JMP);
            DEFINE_LOWERING(LDPC);
            DEFINE_LOWERING(LDREG);
            DEFINE_LOWERING(LDMEM);
			DEFINE_LOWERING(MOV);
			DEFINE_LOWERING(OR);
            DEFINE_LOWERING(RET);
            DEFINE_LOWERING(SCM);
            DEFINE_LOWERING(SHL);
            DEFINE_LOWERING(SAR);
            DEFINE_LOWERING(SHR);
            DEFINE_LOWERING(STREG);
            DEFINE_LOWERING(STMEM);
            DEFINE_LOWERING(SUB);
            DEFINE_LOWERING(TRUNC);
            DEFINE_LOWERING(UMULL);
            DEFINE_LOWERING(XOR);
            DEFINE_LOWERING(ZNFLAGS);
            DEFINE_LOWERING(ZX);
#undef DEFINE_LOWERING
        }
    }
}

#endif /* BLOCKJITADAPTORLOWERING_H */

