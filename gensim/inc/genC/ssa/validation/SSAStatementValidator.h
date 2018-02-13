/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SSAStatementValidator.h
 * Author: harry
 *
 * Created on 29 September 2017, 10:24
 */

#ifndef SSASTATEMENTVALIDATOR_H
#define SSASTATEMENTVALIDATOR_H

#include <vector>

namespace gensim {
	class DiagnosticContext;
	
	namespace genc {
		namespace ssa {
			class SSAStatement;
			namespace validation {
				class SSAStatementValidationPass;
				
				class SSAStatementValidator {
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

