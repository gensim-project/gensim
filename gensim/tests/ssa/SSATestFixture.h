/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SSATestFixture.h
 * Author: harry
 *
 * Created on 04 October 2017, 11:08
 */

#ifndef SSATESTFIXTURE_H
#define SSATESTFIXTURE_H

#include <gtest/gtest.h>

#include "arch/testing/TestArch.h"
#include "genC/ssa/testing/TestContext.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/passes/SSAPass.h"
#include "genC/ssa/SSABlock.h"
#include "genC/ssa/validation/SSAStatementValidationPass.h"

using namespace gensim::genc::ssa::testing;
using namespace gensim::genc::ssa;
using namespace gensim::genc::ssa::validation;

class SSATestFixture : public ::testing::Test { 
public:
	SSATestFixture();
	
	bool RunPass(SSAFormAction* action, SSAStatementValidationPass *pass);

	SSAFormAction *CompileAsm(const std::string &src, const std::string &action_name);
	SSAContext *CompileAsm(const std::string &src);
	
	bool TryAsmWithPass(const std::string &src, gensim::genc::ssa::validation::SSAStatementValidationPass *pass);
	
	gensim::DiagnosticContext &Diag() { return diag_ctx_; }
	
	gensim::arch::ArchDescription *GetTestArch() { return test_arch_; }
	gensim::genc::ssa::SSAContext *GetSSACtx() { return test_context_; }
	
private:
	gensim::arch::ArchDescription *test_arch_;
	gensim::genc::ssa::SSAContext *test_context_;
	
	gensim::DiagnosticSource diag_src_;
	gensim::DiagnosticContext diag_ctx_;
};

#endif /* SSATESTFIXTURE_H */

