/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <set>

#include "isa/InstructionDescriptionParser.h"

#include "define.h"

using namespace gensim::isa;

bool InstructionDescriptionParser::load_constraints_from_node(const ArchC::AstNode &tree, std::list<std::vector<InstructionDescription::DecodeConstraint> > &target)
{
	bool success = true;
	std::vector<InstructionDescription::DecodeConstraint> node_constraints;

	std::set<std::string> constrained_fields;

	for (auto childPtr : tree) {
		auto &child = *childPtr;

		auto &nameNode = child[1];
		auto &valueNode = child[2];
		auto &operatorNode = child[0];

		std::string fieldname = nameNode[0].GetString();

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
		switch (valueNode.GetType()) {
			case ArchCNodeType::INT:
				value = valueNode.GetInt();
				break;
			default:
				UNEXPECTED;
		}

		std::string op = operatorNode.GetString();
		if(op == "=") {
			node_constraints.push_back(InstructionDescription::DecodeConstraint::EqualsConstraint(fieldname, value));
		} else if(op == "!=") {
			node_constraints.push_back(InstructionDescription::DecodeConstraint::NotEqualsConstraint(fieldname, value));
		} else if(op == "&") {
			node_constraints.push_back(InstructionDescription::DecodeConstraint::BitwiseAndConstraint(fieldname, value));
		} else if(op == "^") {
			node_constraints.push_back(InstructionDescription::DecodeConstraint::XOrConstraint(fieldname, value));
		} else {
			UNEXPECTED;
		}
	}

	target.push_back(node_constraints);

	return success;
}
