/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef LLVMREGISTEROPTIMISATIONPASS_H__
#define LLVMREGISTEROPTIMISATIONPASS_H__

#include "define.h"
#include <llvm/Pass.h>
#include <llvm/Analysis/RegionInfo.h>

#include <unordered_set>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class RegisterExtent : public std::pair<int, int>
			{
			public:
				RegisterExtent(int start, int end) : std::pair<int, int>(start, end)
				{
					ASSERT(start <= end);
				}
			};

			class RegisterReference
			{
			public:
				using Extents = RegisterExtent;
				static RegisterReference::Extents UnlimitedExtent()
				{
					return {0, INT_MAX};
				}

				RegisterReference() : extents_(UnlimitedExtent()), precise_(false) {}
				RegisterReference(Extents extents, bool precise) : extents_(extents), precise_(precise) {}

				Extents GetExtents() const
				{
					return extents_;
				}
				size_t GetSize() const
				{
					return extents_.second - extents_.first + 1;
				}
				bool IsPrecise() const
				{
					return precise_;
				}
				bool IsUnlimited() const
				{
					return extents_.first == 0 && extents_.second == INT_MAX;
				}

			private:
				Extents extents_;
				bool precise_;
			};

			class RegisterAccess
			{
			public:
				static RegisterAccess Store(llvm::Instruction *llvm_inst, const RegisterReference &reg)
				{
					return RegisterAccess(llvm_inst, reg, true);
				}
				static RegisterAccess Load(llvm::Instruction *llvm_inst, const RegisterReference &reg)
				{
					return RegisterAccess(llvm_inst, reg, false);
				}

				bool IsStore() const
				{
					return is_store_;
				}
				const RegisterReference &GetRegRef() const
				{
					return reg_;
				}
				llvm::Instruction *GetInstruction() const
				{
					return llvm_inst_;
				}

			protected:
				RegisterAccess() = delete;
				RegisterAccess(llvm::Instruction *llvm_inst, const RegisterReference &reg, bool is_store) : is_store_(is_store), llvm_inst_(llvm_inst), reg_(reg) {}

			private:
				bool is_store_;

				llvm::Instruction *llvm_inst_;
				RegisterReference reg_;
			};


			class LLVMRegisterOptimisationPass : public llvm::FunctionPass
			{
			public:
				LLVMRegisterOptimisationPass();
				bool runOnFunction(llvm::Function & F) override;
				void getAnalysisUsage(llvm::AnalysisUsage & ) const override;

				static char ID;

				using DefinitionSet = std::unordered_set<llvm::Instruction*>;

			private:
				bool ProcessFunction(llvm::Function &f, std::vector<RegisterAccess*> &all_definitions, std::vector<RegisterAccess*> &live_definitions);
			};


		}
	}
}
#endif
