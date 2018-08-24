/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   InstructionDescriptionParser.h
 * Author: harry
 *
 * Created on 12 April 2018, 15:20
 */

#ifndef INSTRUCTIONDESCRIPTIONPARSER_H
#define INSTRUCTIONDESCRIPTIONPARSER_H

#include "isa/InstructionDescription.h"
#include "util/AntlrWrapper.h"
#include "DiagnosticContext.h"

namespace gensim
{
	namespace isa
	{

		class InstructionDescriptionParser
		{
		public:
			static bool load_constraints_from_node(gensim::AntlrTreeWrapper tree, std::list<std::vector<InstructionDescription::DecodeConstraint> > &target);
		};
	}
}

#endif /* INSTRUCTIONDESCRIPTIONPARSER_H */

