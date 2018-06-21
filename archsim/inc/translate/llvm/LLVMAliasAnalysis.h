/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   LLVMAliasAnalysis.h
 * Author: s0457958
 *
 * Created on 18 July 2014, 13:04
 */

#ifndef LLVMALIASANALYSIS_H
#define	LLVMALIASANALYSIS_H

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
			};
		}
	}
}

#endif	/* LLVMALIASANALYSIS_H */

