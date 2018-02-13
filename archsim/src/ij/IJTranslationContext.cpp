/*
 * ij/IJTranslationContext.cpp
 */
#include "ij/IJTranslationContext.h"
#include "ij/IJBlock.h"
#include "ij/ir/IRBuilder.h"
#include "ij/arch/x86/X86Compiler.h"

#include "gensim/gensim_processor.h"
#include "gensim/gensim_translate.h"

#include "util/SimOptions.h"
#include "util/LogContext.h"

#include <iostream>
#include <sys/mman.h>

UseLogContext(LogIJ)

using namespace archsim::ij;

IJTranslationContext::IJTranslationContext(IJManager& mgr, IJBlock& block, gensim::Processor& cpu) : mgr(mgr), block(block), cpu(cpu), ir(1)
{

}

IJManager::ij_block_fn IJTranslationContext::Translate()
{
	if (!GenerateIJIR()) {
		LC_ERROR(LogIJ) << "Error generating IJIR";
		return NULL;
	}

	if (!OptimiseIJIR()) {
		LC_ERROR(LogIJ) << "Error optimising IJIR";
		return NULL;
	}

	IJManager::ij_block_fn fnp;
	if (!CompileIJIR(fnp)) {
		LC_ERROR(LogIJ) << "Error compiling IJIR";
		return NULL;
	}

	LC_DEBUG1(LogIJ) << "Successfully compiled IJ function @ " << std::hex << (unsigned long)fnp;

	return fnp;
}

bool IJTranslationContext::GenerateIJIR()
{
	gensim::BaseIJTranslate *txl = cpu.CreateIJTranslate(*this);

	for (auto insn : block.GetInstructions()) {
		if (!txl->TranslateInstruction(*this, *insn.first, insn.second, archsim::options::Trace.IsSpecified())) {
			delete txl;
			return false;
		}
	}

	ir.GetBuilder().CreateAdd(ir.GetParameterValue(0), ir.GetConstantInt32(5));
	ir.GetBuilder().CreateRet(ir.GetConstantInt32(0));

	delete txl;
	return true;
}

bool IJTranslationContext::OptimiseIJIR()
{
	return true;
}

bool IJTranslationContext::CompileIJIR(IJManager::ij_block_fn& fnp)
{
	uint8_t *buffer = (uint8_t *)mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	LC_DEBUG1(LogIJ) << "Allocated code buffer @ 0x" << std::hex << (unsigned long)buffer;

	arch::x86::X86Compiler compiler;
	if (compiler.Compile(ir, buffer) == 0) {
		free(buffer);
		return false;
	}

	fnp = (IJManager::ij_block_fn)buffer;
	return true;
}
