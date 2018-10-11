/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef LLVMREGISTEROPTIMISATIONPASS_H__
#define LLVMREGISTEROPTIMISATIONPASS_H__

#include <llvm/Pass.h>
#include <llvm/Analysis/RegionInfo.h>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMRegisterOptimisationPass : public llvm::FunctionPass
			{
			public:
				LLVMRegisterOptimisationPass();
				bool runOnFunction(llvm::Function & F) override;
				void getAnalysisUsage(llvm::AnalysisUsage & ) const override;

				static char ID;

			private:
				bool runOnRegion(llvm::Function &f, const llvm::Region *region);
			};


		}
	}
}
#endif
