/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "gensim/gensim_translate.h"

using namespace gensim;

bool BaseLLVMTranslate::EmitRegisterRead(void* irbuilder, int size, int offset)
{
	UNIMPLEMENTED;
}

bool BaseLLVMTranslate::EmitRegisterWrite(void* irbuilder, int size, int offset, llvm::Value *value)
{
	UNIMPLEMENTED;
}

llvm::Value* BaseLLVMTranslate::EmitMemoryRead(void* irbuilder, int size, llvm::Value* address)
{
	UNIMPLEMENTED;
}

void BaseLLVMTranslate::EmitMemoryWrite(void* irbuilder, int size, llvm::Value* address, llvm::Value* value)
{
	UNIMPLEMENTED;
}
