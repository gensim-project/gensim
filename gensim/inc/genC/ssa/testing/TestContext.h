/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   TestContext.h
 * Author: harry
 *
 * Created on 16 August 2017, 16:27
 */

#ifndef TESTCONTEXT_H
#define TESTCONTEXT_H

namespace gensim
{
	class DiagnosticContext;
	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
			class SSAFormAction;
			namespace testing
			{
				// Completely build a test SSA context, based on the test isa
				// and test architecture
				SSAContext *GetTestContext();

				bool AssembleTest(SSAContext *ctx, const std::string &assembly, gensim::DiagnosticContext &dc);
				void DisassembleAction(SSAFormAction *action);
			}
		}
	}
}

#endif /* TESTCONTEXT_H */

