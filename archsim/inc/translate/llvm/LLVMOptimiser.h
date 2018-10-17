/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMOptimiser.h
 * Author: s0457958
 *
 * Created on 21 July 2014, 15:46
 */

#ifndef LLVMOPTIMISER_H
#define	LLVMOPTIMISER_H

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Analysis/AliasAnalysis.h>

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
		namespace translate_llvm
		{

			class ArchSimAA : public llvm::AAResultBase<ArchSimAA>
			{
				static char ID;
			public:
				ArchSimAA();

				virtual llvm::AliasResult alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2);

			private:
				llvm::AliasResult do_alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2);
			};

			class LLVMOptimiser
			{
			public:
				LLVMOptimiser();
				~LLVMOptimiser();

				bool Optimise(::llvm::Module *module, const ::llvm::DataLayout &data_layout);

			private:
				bool Initialise(const ::llvm::DataLayout&);

				bool AddPass(::llvm::Pass*);

				::llvm::legacy::PassManager pm;
				ArchSimAA *my_aa_;
				bool isInitialised;
			};
		}
	}
}

#endif	/* LLVMOPTIMISER_H */

