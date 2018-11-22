/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef LLVMREGISTEROPTIMISATIONPASS_H__
#define LLVMREGISTEROPTIMISATIONPASS_H__

#include <llvm/Pass.h>
#include <llvm/Analysis/RegionInfo.h>

#include <unordered_set>

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

				using DefinitionSet = std::unordered_set<llvm::Instruction*>;

			private:
				bool ProcessFunction(llvm::Function &f, DefinitionSet &all_definitions, DefinitionSet &live_definitions);
			};


		}
	}
}
#endif
