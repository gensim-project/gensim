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
			class ArchSimAA : public llvm::AliasAnalysis, llvm::FunctionPass
			{
			public:
				static char ID;

				ArchSimAA() : FunctionPass(ID) {}

				bool runOnFunction(llvm::Function &F) override;
				
				
				void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;				
				virtual llvm::AliasResult alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2);

			private:
				llvm::AliasResult do_alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2);
			};
			
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

