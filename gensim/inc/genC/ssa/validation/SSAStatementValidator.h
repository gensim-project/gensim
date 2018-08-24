/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SSAStatementValidator.h
 * Author: harry
 *
 * Created on 29 September 2017, 10:24
 */

#ifndef SSASTATEMENTVALIDATOR_H
#define SSASTATEMENTVALIDATOR_H

#include <vector>

namespace gensim
{
	class DiagnosticContext;

	namespace genc
	{
		namespace ssa
		{
			class SSAStatement;
			namespace validation
			{
				class SSAStatementValidationPass;

				class SSAStatementValidator
				{
				public:
					SSAStatementValidator();
					bool Run(SSAStatement *stmt, DiagnosticContext &diag);

				private:
					std::vector<SSAStatementValidationPass*> passes_;
				};
			}
		}
	}
}

#endif /* SSASTATEMENTVALIDATOR_H */

