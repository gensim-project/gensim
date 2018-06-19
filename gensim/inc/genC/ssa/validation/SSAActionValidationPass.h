/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   SSAValidationPass.h
 * Author: harry
 *
 * Created on 26 September 2017, 13:23
 */

#pragma once

#include <vector>

namespace gensim
{
	class DiagnosticContext;

	namespace genc
	{
		namespace ssa
		{
			class SSAFormAction;
			class SSAStatement;

			namespace validation
			{
				class SSAStatementValidationPass;
				class SSAActionValidationPass
				{
				public:
					virtual ~SSAActionValidationPass();
					virtual bool Run(const SSAFormAction *action, DiagnosticContext &ctx) = 0;
				};

				class SSAValidationManager
				{
				public:
					bool Run(SSAFormAction *action, DiagnosticContext& ctx);
					bool Run(SSAStatement *stmt, DiagnosticContext &ctx);
				private:
					std::vector<SSAActionValidationPass*> action_passes_;
					std::vector<SSAStatementValidationPass*> statement_passes_;
				};
			}
		}
	}
}
