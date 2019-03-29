/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include <gtest/gtest.h>

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/Parser.h"
#include "genC/ssa/analysis/LoopAnalysis.h"
#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/validation/SSAStatementValidationPass.h"

#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/ssa/io/AssemblyReader.h"

using namespace gensim::genc::ssa::testing;

TEST(SSA_Assembler_Reassemble, ExternalFunction)
{
// Disassemble, and then reassemble, an action containing a call to
// an external function

	std::string sourcecode = R"||(
		helper void test() { flush_itlb_entry(0); }
	)||";

	// compile
	gensim::DiagnosticSource root_source("GenSim");
	gensim::DiagnosticContext root_context(root_source);

	auto gencctx = gensim::genc::testing::TestContext::GetTestContext(false, root_context);
	auto ctx = gensim::genc::testing::TestContext::CompileSource(gencctx, sourcecode);

	ASSERT_NE(nullptr, ctx);

// disassemble code
	gensim::genc::ssa::io::ContextDisassembler cd;
	std::stringstream str;
	cd.Disassemble(ctx, str);

// reassemble code
	gensim::genc::ssa::io::AssemblyReader ar;
	gensim::genc::ssa::io::AssemblyFileContext *afc;
	bool success = ar.ParseText(str.str(), root_context, afc);

	ASSERT_EQ(true, success);

	gensim::genc::ssa::SSAContext *out_ctx = new gensim::genc::ssa::SSAContext(gencctx->ISA, gencctx->Arch);
	gensim::genc::ssa::io::ContextAssembler ca;
	ca.SetTarget(out_ctx);
	bool result = ca.Assemble(*afc, root_context);

	ASSERT_EQ(true, result);

}
