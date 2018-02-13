/*
 * FieldDescription.cpp
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#include "isa/FieldDescription.h"

#include <antlr3.h>

using namespace gensim;
using namespace gensim::isa;

FieldDescriptionParser::FieldDescriptionParser(DiagnosticContext &diag) : diag(diag), description(new FieldDescription()) {}

bool FieldDescriptionParser::Parse(void *ptree)
{
	pANTLR3_BASE_TREE tree = (pANTLR3_BASE_TREE)ptree;
	FieldDescription &rval = *description;
	pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
	pANTLR3_BASE_TREE typeNode = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);

	rval.field_name = std::string((char *)nameNode->getText(nameNode)->chars);
	rval.field_type = std::string((char *)nameNode->getText(typeNode)->chars);

	/*if (rval.field_type.find("int", 0) != rval.field_type.npos)
		rval.field_type.append("_t");*/

	if (typeNode->getChildCount(typeNode) > 0) {
		pANTLR3_BASE_TREE modNode = (pANTLR3_BASE_TREE)typeNode->getChild(typeNode, 0);
		rval.field_type.append((char *)modNode->getText(modNode)->chars);
	}

	return true;
}

FieldDescription *FieldDescriptionParser::Get()
{
	return description;
}
