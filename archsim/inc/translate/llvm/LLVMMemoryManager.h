/*
 * LLVMMemoryManager.h
 *
 *  Created on: 6 Nov 2014
 *      Author: harry
 */

#ifndef LLVMMEMORYMANAGER_H_
#define LLVMMEMORYMANAGER_H_

#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <vector>
#include "util/PagePool.h"

namespace archsim
{
	namespace translate
	{
		namespace translate_llvm
		{

			class LLVMMemoryManager : public ::llvm::RTDyldMemoryManager
			{
			public:
				LLVMMemoryManager(util::PagePool &code_pool, util::PagePool &data_pool);
				~LLVMMemoryManager();

				virtual uint8_t* allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, ::llvm::StringRef SectionName) override;
				virtual uint8_t* allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, ::llvm::StringRef SectionName, bool IsReadOnly) override;

				inline uint64_t getAllocatedCodeSize() const
				{
					return code_size;
				}
				inline uint64_t getAllocatedDataSize() const
				{
					return data_size;
				}

				virtual bool finalizeMemory(std::string *ErrMsg) override;

				std::vector<util::PageReference *> ReleasePages();

			private:
				util::PagePool &code_pool, &data_pool;
				std::vector<util::PageReference *> code_pages;
				std::vector<util::PageReference *> data_pages;

				uint64_t code_size, data_size;
			};

		}
	}
}


#endif /* LLVMMEMORYMANAGER_H_ */
