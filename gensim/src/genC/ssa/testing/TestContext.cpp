/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "define.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "arch/testing/TestArch.h"
#include "isa/testing/TestISA.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/InstStructBuilder.h"

#include <iostream>

using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::testing;

SSAContext *gensim::genc::ssa::testing::GetTestContext()
{
	auto isa = gensim::isa::testing::GetTestISA(false);
	auto ctx = new SSAContext(*isa, *gensim::arch::testing::GetTestArch());
	gensim::genc::InstStructBuilder isb;

	ctx->GetTypeManager().InsertStructType("Instruction", isb.BuildType(isa));

	return ctx;
}

bool gensim::genc::ssa::testing::AssembleTest(SSAContext *ctx, const std::string &assembly, gensim::DiagnosticContext &dc)
{
	io::ContextAssembler ca;
	ca.SetTarget(ctx);

	gensim::genc::ssa::io::AssemblyFileContext *asm_ctx;
	io::AssemblyReader ar;
	bool parsed = ar.ParseText(assembly, dc, asm_ctx);
	if(!parsed) {
		return false;
	}
	bool assembled = ca.Assemble(*asm_ctx, dc);
	return assembled;
}

void gensim::genc::ssa::testing::DisassembleAction(SSAFormAction* action)
{
	gensim::genc::ssa::io::ActionDisassembler ad;
	ad.Disassemble(action, std::cout);
}
