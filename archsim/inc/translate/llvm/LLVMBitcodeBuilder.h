/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMBitcodeBuilder.h
 * Author: harry
 *
 * Created on 25 July 2018, 15:49
 */

#ifndef LLVMBITCODEBUILDER_H
#define LLVMBITCODEBUILDER_H

#include <llvm/IR/IRBuilder.h>

namespace archsim
{
	namespace translate
	{
		namespace llvm
		{
			class LLVMBitcodeBuilder
			{
			public:
				LLVMBitcodeBuilder(llvm::IRBuilder<> &builder) : builder_(builder) {}

				virtual bool AddInstruction(Address addr, gensim::BaseDecode *insn) = 0;

			protected:
				llvm::IRBuilder<> &builder_;
			};
		}
	}
}

#endif /* LLVMBITCODEBUILDER_H */

