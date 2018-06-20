/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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
#include "translate/llvm/LLVMOptimiser.h"

namespace archsim
{
	namespace translate
	{
		namespace adapt
		{

			::llvm::FunctionType *GetBlockFunctionType(::llvm::LLVMContext &ctx);

			class BlockJITToLLVMAdaptor
			{
			public:
				BlockJITToLLVMAdaptor(::llvm::LLVMContext &ctx);
				::llvm::Function *AdaptIR(archsim::core::thread::ThreadInstance *thread, ::llvm::Module *target_module, const std::string &name, const captive::arch::jit::TranslationContext &ctx);

			private:
				::llvm::LLVMContext &ctx_;
				translate_llvm::LLVMOptimiser optimiser_;
			};
		}
	}
}

#endif /* BLOCKJITTOLLVM_H */

