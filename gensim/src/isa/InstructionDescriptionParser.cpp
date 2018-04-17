/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <archc/archcParser.h>
#include <archc/archcLexer.h>

#include <set>

#include "isa/InstructionDescriptionParser.h"

using namespace gensim::isa;

bool InstructionDescriptionParser::load_constraints_from_node(gensim::AntlrTreeWrapper tree, std::list<std::vector<InstructionDescription::DecodeConstraint> > &target)
{
	pANTLR3_BASE_TREE node = tree.Get();
	bool success = true;
	// first node is identifier node - check that this instruction's name matches the node name
	std::vector<InstructionDescription::DecodeConstraint> node_constraints;
	pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)node->getChild(node, 0);

//	assert(!(strcmp((char *)nameNode->getText(nameNode)->chars, this->Name.c_str())) && "Internal error: attempted to add constraints to non-matching instruction");

	std::set<std::string> constrained_fields;

	for (uint32_t i = 1; i < node->getChildCount(node); i++) {
		pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)node->getChild(node, i);
		pANTLR3_BASE_TREE valueNode = (pANTLR3_BASE_TREE)nameNode->getChild(nameNode, 0);
		pANTLR3_BASE_TREE operatorNode = (pANTLR3_BASE_TREE)nameNode->getChild(nameNode, 1);

		std::string fieldname = std::string((char*)nameNode->getText(nameNode)->chars);

		// check that the format of this instruction has the specified field
		//	if (!Format->hasChunk((char *)nameNode->getText(nameNode)->chars)) {
		//	fprintf(stderr, "Format of instruction %s (%s) does not have a field named '%s' (Line %u)\n", Name.c_str(), Format->GetName().c_str(), nameNode->getText(nameNode)->chars, nameNode->getLine(nameNode));
		//	success = false;
		//}

//		if(constrained_fields.count(fieldname)) {
//			fprintf(stderr, "Format of instruction %s (%s) constrains field %s multiple times\n", Name.c_str(), Format->GetName().c_str(), fieldname.c_str());
//			success = false;
//		}
		constrained_fields.insert(fieldname);

		int value = 0;
		switch (valueNode->getType(valueNode)) {
			case AC_INT:
				value = atoi((char *)valueNode->getText(valueNode)->chars);
				break;
			case AC_HEX_VAL:
				sscanf((char *)valueNode->getText(valueNode)->chars, "0x%x", &value);
				break;
			default:
				fprintf(stderr, "Syntax error on line %u: '%s'\n", valueNode->getLine(valueNode), valueNode->getText(valueNode)->chars);
				return false;
		}

		switch (operatorNode->getType(operatorNode)) {
			case EQUALS:
				node_constraints.push_back(InstructionDescription::DecodeConstraint::EqualsConstraint(fieldname, value));
				break;
			case AMPERSAND:
				node_constraints.push_back(InstructionDescription::DecodeConstraint::BitwiseAndConstraint(fieldname, value));
				break;
			case NOTEQUALS:
				node_constraints.push_back(InstructionDescription::DecodeConstraint::NotEqualsConstraint(fieldname, value));
				break;
			case XOR:
				node_constraints.push_back(InstructionDescription::DecodeConstraint::XOrConstraint(fieldname, value));
				break;
			default:
				assert(!"Unrecognized decode constraint operator!");
		}
	}
	target.push_back(node_constraints);

	return success;
}
