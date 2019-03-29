/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

#include "flexbison_archc_ast.h"
#include "flexbison_archc.h"

namespace gensim
{
	namespace isa
	{

		class AsmDescriptionParser
		{
		public:
			AsmDescriptionParser(DiagnosticContext &diag, std::string filename);

			bool Parse(const ArchC::AstNode &tree, const ISADescription &format);

			AsmDescription *Get();
		private:
			AsmDescription *description;
			DiagnosticContext &diag;
			std::string filename;
		};

	}
}

#endif /* ASMDESCRIPTIONPARSER_H */

