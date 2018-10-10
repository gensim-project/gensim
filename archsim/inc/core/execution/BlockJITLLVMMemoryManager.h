/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   BlockJITLLVMMemoryManager.h
 * Author: harry
 *
 * Created on 27 August 2018, 14:34
 */

#ifndef BLOCKJITLLVMMEMORYMANAGER_H
#define BLOCKJITLLVMMEMORYMANAGER_H

#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			class BlockJITLLVMMemoryManager : public llvm::RTDyldMemoryManager
			{
			public:
				BlockJITLLVMMemoryManager(wulib::MemAllocator &allocator);

				uint8_t* allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName) override;
				uint8_t* allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, llvm::StringRef SectionName, bool IsReadOnly) override;

				bool finalizeMemory(std::string* ErrMsg) override;

				uintptr_t GetSectionSize(uint8_t* ptr) const
				{
					return section_sizes_.at(ptr);
				}
			private:
				wulib::MemAllocator &allocator_;

				std::vector<std::pair<uint8_t*, uintptr_t>> outstanding_code_sections_;
				std::map<uint8_t*, uintptr_t> section_sizes_;

			};
		}
	}
}

#endif /* BLOCKJITLLVMMEMORYMANAGER_H */

