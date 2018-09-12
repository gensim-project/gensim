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



//				void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
				virtual llvm::AliasResult alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2);

			private:
				llvm::AliasResult do_alias(const llvm::MemoryLocation &L1, const llvm::MemoryLocation &L2);
			};


			class RecoverAAInfoPass : public llvm::FunctionPass
			{
			public:
				static char ID;

				RecoverAAInfoPass();
				bool runOnFunction(llvm::Function &F) override;
			};

			class ArchsimAAWrapper : public llvm::FunctionPass
			{
			public:
				static char ID;
				ArchsimAAWrapper();

				bool runOnFunction(llvm::Function &F) override
				{
					auto &TLIWP = getAnalysis<llvm::TargetLibraryInfoWrapperPass>();
					aa_.reset(new ArchSimAA());
				}
				void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
				{
					AU.setPreservesAll();
					AU.addRequired<llvm::TargetLibraryInfoWrapperPass>();
				}

				ArchSimAA &getResult()
				{
					return *aa_;
				}
				const ArchSimAA &getResult() const
				{
					return *aa_;
				}
			private:
				std::unique_ptr<ArchSimAA> aa_;
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

