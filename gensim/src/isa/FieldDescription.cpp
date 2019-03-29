/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "isa/FieldDescription.h"

#include "define.h"

using namespace gensim;
using namespace gensim::isa;

FieldDescriptionParser::FieldDescriptionParser(DiagnosticContext &diag) : diag(diag), description(new FieldDescription()) {}

bool FieldDescriptionParser::Parse(const ArchC::AstNode &ptree)
{
	if(ptree.GetChildren().size() != 2) {
		UNEXPECTED;
	}

	FieldDescription &rval = *description;

	rval.field_name = ptree[0][0].GetString();
	rval.field_type = ptree[1][0].GetString();

	return true;
}

FieldDescription *FieldDescriptionParser::Get()
{
	return description;
}
