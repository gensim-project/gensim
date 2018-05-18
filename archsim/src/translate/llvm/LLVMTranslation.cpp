#include "translate/llvm/LLVMTranslation.h"
#include "translate/llvm/LLVMMemoryManager.h"


#include <llvm/ExecutionEngine/ExecutionEngine.h>

using namespace archsim::translate;
using namespace archsim::translate::translate_llvm;

static uint32_t InvalidTxln(void*, void*)
{
	fprintf(stderr, "Attempted to execute invalid txln\n");
	abort();
}

LLVMTranslation::LLVMTranslation(translation_fn fnp, LLVMMemoryManager *mem_mgr) : fnp(fnp)
{
	if(mem_mgr) {
		code_size = mem_mgr->getAllocatedCodeSize();
		zones = mem_mgr->ReleasePages();
	}
}

LLVMTranslation::~LLVMTranslation()
{
	for(auto zone : zones) delete zone;
	fnp = InvalidTxln;
}

uint32_t LLVMTranslation::Execute(archsim::core::thread::ThreadInstance* cpu)
{
	return fnp(nullptr, nullptr);
}

void LLVMTranslation::Install(translation_fn *location)
{
	*location = (translation_fn)fnp;
}

uint32_t LLVMTranslation::GetCodeSize() const
{
	return code_size;
}
