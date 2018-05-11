/*
 * File:   LLVMOptimiser.h
 * Author: s0457958
 *
 * Created on 21 July 2014, 15:46
 */

#ifndef LLVMOPTIMISER_H
#define	LLVMOPTIMISER_H

#include <llvm/IR/LegacyPassManager.h>

namespace llvm
{
	class Module;
	class DataLayout;
	class Pass;
}

namespace archsim
{
	namespace translate
	{
		namespace llvm
		{
			class LLVMOptimiser
			{
			public:
				LLVMOptimiser();
				~LLVMOptimiser();

				bool Optimise(::llvm::Module *module, const ::llvm::DataLayout *data_layout);

			private:
				bool Initialise(const ::llvm::DataLayout*);

				bool AddPass(::llvm::Pass*);

				::llvm::legacy::PassManager pm;
				bool isInitialised;
			};
		}
	}
}

#endif	/* LLVMOPTIMISER_H */

