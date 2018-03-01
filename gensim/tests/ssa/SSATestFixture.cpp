/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "SSATestFixture.h"

#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/io/Assembler.h"
#include "genC/ssa/io/AssemblyReader.h"
#include "arch/testing/TestArch.h"
#include "isa/testing/TestISA.h"
#include "genC/ssa/io/Disassemblers.h"
#include "genC/InstStructBuilder.h"


using namespace gensim::genc::ssa;

SSATestFixture::SSATestFixture() : test_arch_(gensim::arch::testing::GetTestArch()), diag_src_("test"), diag_ctx_(diag_src_)
{
	gensim::isa::ISADescription *isa = gensim::isa::testing::GetTestISA(false);
	test_context_ = new SSAContext(*isa, *test_arch_);
	gensim::genc::InstStructBuilder isb;

	test_context_->GetTypeManager().InsertStructType("Instruction", isb.BuildType(isa));
}

SSAFormAction* SSATestFixture::CompileAsm(const std::string& src, const std::string &action_name)
{
	auto test_ctx = GetTestContext();

	io::ContextAssembler ca;
	ca.SetTarget(test_ctx);

	gensim::genc::ssa::io::AssemblyFileContext *asm_ctx;
	io::AssemblyReader ar;
	bool parsed = ar.ParseText(src, Diag(), asm_ctx);
	EXPECT_EQ(parsed, true);
	if (!parsed) {
		return nullptr;
	}
	bool assembled = ca.Assemble(*asm_ctx, Diag());
	if (!assembled) {
		std::cerr << Diag();
		return nullptr;
	}
	EXPECT_EQ(assembled, true);


	auto test_action = (gensim::genc::ssa::SSAFormAction*)test_ctx->GetAction(action_name);
	return test_action;
}

SSAContext* SSATestFixture::CompileAsm(const std::string& src)
{
	auto test_ctx = GetTestContext();

	io::ContextAssembler ca;
	ca.SetTarget(test_ctx);

	gensim::genc::ssa::io::AssemblyFileContext *asm_ctx;
	io::AssemblyReader ar;
	bool parsed = ar.ParseText(src, Diag(), asm_ctx);
	EXPECT_EQ(parsed, true);
	if (!parsed) {
		return nullptr;
	}
	bool assembled = ca.Assemble(*asm_ctx, Diag());
	if (!assembled) {
		std::cerr << Diag();
		return nullptr;
	}
	EXPECT_EQ(assembled, true);


	return test_ctx;
}

bool SSATestFixture::RunPass(SSAFormAction* action, SSAStatementValidationPass* pass)
{
	for (auto block : action->Blocks) {
		for (auto stmt : block->GetStatements()) {
			if (!pass->Run(stmt, Diag())) {
				return false;
			}
		}
	}
	return true;
}

bool SSATestFixture::TryAsmWithPass(const std::string& src, gensim::genc::ssa::validation::SSAStatementValidationPass* pass)
{
	auto test_action = CompileAsm(src, "test_action");

	return RunPass(test_action, pass);
}
