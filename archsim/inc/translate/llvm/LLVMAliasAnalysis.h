/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMAliasAnalysis.h
 * Author: s0457958
 *
 * Created on 18 July 2014, 13:04
 */

#ifndef LLVMALIASANALYSIS_H
#define	LLVMALIASANALYSIS_H

#include <llvm/IR/Value.h>
#include <vector>

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{
			enum AliasAnalysisTag {
				TAG_REG_ACCESS = 0,
				TAG_REG_BANK_ACCESS = 1,
				TAG_MEM_ACCESS = 2,
				TAG_CPU_STATE = 3,
				TAG_JT_ELEMENT = 4,
				TAG_REGION_CHAIN_TABLE = 5,
				TAG_METRIC = 6,
				TAG_SPARSE_PAGE_MAP = 7,
				TAG_MMAP_ENTRY = 8,

				TAG_SMM_CACHE = 9,
				TAG_SMM_PAGE  = 10,

				TAG_LOCAL = 11
			};

			class PointerInformationProvider
			{
			public:
				using PointerInfo = std::vector<int>;

				PointerInformationProvider(llvm::Function *fn);

				bool GetPointerInfo(llvm::Value *v, int size, PointerInfo &output_aaai) const;
				bool GetPointerInfo(llvm::Value *v, PointerInfo &output_aaai) const;

				bool GetPointerInfo(const llvm::Value *v, int size, PointerInfo &output_aaai) const;
				bool GetPointerInfo(const llvm::Value *v, PointerInfo &output_aaai) const;

				llvm::Value *GetStateBase() const;
				llvm::Value *GetMemBase() const;
				llvm::Value *GetRegBase() const;

			private:
				void DecodeMetadata(const llvm::Instruction *i, PointerInfo &pi) const;
				void EncodeMetadata(llvm::Instruction *i, const PointerInfo &pi) const;

				bool TryRecover_Chain(const llvm::Value *v, int size, std::vector<int> &output_aaai) const;
				bool TryRecover_MemAccess(const llvm::Value *v, PointerInfo &pi) const;
				bool TryRecover_FromPhi(const llvm::Value *v, int size, std::vector<int> &aaai) const;
				bool TryRecover_FromSelect(const llvm::Value *v, int size, std::vector<int> &aaai) const;
				bool TryRecover_StateBlock(const llvm::Value *v, std::vector<int> &aaai) const;

				llvm::Function *function_;
			};
		}
	}
}

#endif	/* LLVMALIASANALYSIS_H */

