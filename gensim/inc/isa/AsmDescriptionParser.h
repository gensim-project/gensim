/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AsmDescriptionParser.h
 * Author: harry
 *
 * Created on 12 April 2018, 15:09
 */

#ifndef ASMDESCRIPTIONPARSER_H
#define ASMDESCRIPTIONPARSER_H

#include "DiagnosticContext.h"
#include "isa/ISADescription.h"
#include "isa/AsmDescription.h"
#include <string>

namespace gensim {
	namespace isa {
		
		class AsmDescriptionParser
		{
		public:
			AsmDescriptionParser(DiagnosticContext &diag, std::string filename);

			bool Parse(void *tree, const ISADescription &format);

			AsmDescription *Get();
		private:
			AsmDescription *description;
			DiagnosticContext &diag;
			std::string filename;
		};

	}
}

#endif /* ASMDESCRIPTIONPARSER_H */

