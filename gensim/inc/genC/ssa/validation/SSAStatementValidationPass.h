/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SSAValidationVisitorPass.h
 * Author: harry
 *
 * Created on 28 September 2017, 10:50
 */

#ifndef _GENC_SSA_VALIDATION_SSAVALIDATIONVISITORPASS_H_
#define _GENC_SSA_VALIDATION_SSAVALIDATIONVISITORPASS_H_

#include "genC/ssa/validation/SSAActionValidationPass.h"
#include "genC/ssa/SSAStatementVisitor.h"

#include <string>

namespace gensim
{
	class DiagNode;
	class DiagnosticContext;

	namespace genc
	{
		namespace ssa
		{
			namespace validation
			{
				class SSAStatementValidationPass : public EmptySSAStatementVisitor
				{
				public:
					bool Run(SSAStatement* statement, DiagnosticContext& ctx);

					// Override default behaviour in order to throw an UNIMPLEMENTED here
					void VisitStatement(SSAStatement& stmt) override;

				protected:
					void Fail(const std::string message);
					void Fail(const std::string &message, const DiagNode &diag);
					void Assert(bool expression, const std::string &message, const DiagNode &diag);
					DiagnosticContext &Diag();

				private:
					DiagnosticContext *diag_;
					bool success_;
				};
			}
		}
	}
}

#endif /* _GENC_SSA_VALIDATION_SSAVALIDATIONVISITORPASS_H_ */

