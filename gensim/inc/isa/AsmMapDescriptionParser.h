/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   AsmMapDescriptionParser.h
 * Author: harry
 *
 * Created on 12 April 2018, 15:15
 */

#ifndef ASMMAPDESCRIPTIONPARSER_H
#define ASMMAPDESCRIPTIONPARSER_H

#include "isa/AsmMapDescription.h"

#include "flexbison_harness.h"
#include "flexbison_archc_ast.h"
#include "flexbison_archc.h"

namespace gensim
{
	namespace isa
	{
		class AsmMapDescriptionParser
		{
		public:
			static AsmMapDescription Parse(const ArchC::AstNode &pnode);
		};
	}
}

#endif /* ASMMAPDESCRIPTIONPARSER_H */

