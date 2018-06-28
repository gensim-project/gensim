/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LLVMMemoryManager.cpp
 *
 *  Created on: 6 Nov 2014
 *      Author: harry
 */

#include "translate/llvm/LLVMMemoryManager.h"

#include <cassert>
#include <sys/mman.h>

using namespace archsim::util;
using namespace archsim::translate::translate_llvm;

LLVMMemoryManager::LLVMMemoryManager(util::PagePool &code_pool, util::PagePool &data_pool) : code_pool(code_pool), data_pool(data_pool), code_size(0),data_size(0)
{

}

LLVMMemoryManager::~LLVMMemoryManager()
{
	for(auto page : code_pages) delete page;
	for(auto page : data_pages) delete page;
}

uint8_t *LLVMMemoryManager::allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, ::llvm::StringRef SectionName)
{
	auto code_page = code_pool.AllocateB(Size);
	code_pages.push_back(code_page);
	code_size += Size;
	return (uint8_t*)code_page->Data;
}

uint8_t *LLVMMemoryManager::allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, ::llvm::StringRef SectionName, bool IsReadOnly)
{
	auto data_page = data_pool.AllocateB(Size);
	data_pages.push_back(data_page);
	data_size += Size;
	return (uint8_t*)data_page->Data;
}

bool LLVMMemoryManager::finalizeMemory(std::string *ErrMsg)
{
	for(auto page : data_pages) mprotect(page->Data, page->Size(), PROT_READ | PROT_WRITE);
	for(auto page : code_pages) mprotect(page->Data, page->Size(), PROT_READ | PROT_EXEC);
	return false;
}

std::vector<archsim::util::PageReference*> LLVMMemoryManager::ReleasePages()
{
	std::vector<archsim::util::PageReference*> pages;
	pages.insert(pages.begin(), data_pages.begin(), data_pages.end());
	pages.insert(pages.begin(), code_pages.begin(), code_pages.end());

	data_pages.clear();
	code_pages.clear();

	return pages;
}

