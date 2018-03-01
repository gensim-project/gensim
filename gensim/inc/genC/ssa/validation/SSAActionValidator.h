/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SSAActionValidator.h
 * Author: harry
 *
 * Created on 29 September 2017, 10:23
 */

#ifndef SSAACTIONVALIDATOR_H
#define SSAACTIONVALIDATOR_H

#include "genC/ssa/validation/SSAStatementValidator.h"

#include <vector>

namespace gensim
{
	class DiagnosticContext;

	namespace genc
	{
		namespace ssa
		{
			class SSAFormAction;
			namespace validation
			{
				class SSAActionValidationPass;

				class SSAActionValidator
				{
				public:
					SSAActionValidator();
					bool Run(SSAFormAction *action, DiagnosticContext &diag);
				private:
					std::vector<SSAActionValidationPass*> passes_;
					SSAStatementValidator statement_validator_;
				};
			}
		}
	}
}

#endif /* SSAACTIONVALIDATOR_H */

