/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockJITToLLVM.h
 * Author: harry
 *
 * Created on 14 May 2018, 11:57
 */

#ifndef BLOCKJITTOLLVM_H
#define BLOCKJITTOLLVM_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include <string>

#include "blockjit/translation-context.h"
#include "blockjit/IRInstruction.h"

namespace archsim {
    namespace translate {
        namespace adapt {
			
			::llvm::FunctionType *GetBlockFunctionType(::llvm::LLVMContext &ctx);
			::llvm::FunctionType *GetIRBlockFunctionType(::llvm::LLVMContext &ctx);
			
            class BlockJITToLLVMAdaptor {
            public:	
				BlockJITToLLVMAdaptor(::llvm::LLVMContext &ctx);
                ::llvm::Function *AdaptIR(::llvm::Module *target_module, const std::string &name, const captive::arch::jit::TranslationContext &ctx);
				
			private:
				::llvm::LLVMContext &ctx_;
            };
        }
    }
}

#endif /* BLOCKJITTOLLVM_H */

