/*
 * InstructionDescription.cpp
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#include "isa/InstructionDescription.h"
#include "Util.h"

#include <archc/archcParser.h>
#include <archc/archcLexer.h>

#include <cstring>
#include <string>
#include <set>
#include <unordered_map>

using namespace gensim;
using namespace gensim::isa;

std::vector<std::string> ConstrainChunk(uint32_t chunk_length, const std::vector<const InstructionDescription::DecodeConstraint*> &constraints)
{
	std::vector<uint32_t> values;
	values.reserve(1 << chunk_length);

	//First, generate possible chunks
	std::stringstream str;
	for(uint32_t i = 0; i < 1 << chunk_length; ++i) {
		values.push_back(i);
	}

	std::vector<std::string> output;
	//Now, check each against the given constraints and return those which pass
	for(uint32_t value : values) {
		for(const auto constraint : constraints) {
			switch(constraint->Type) {
				case InstructionDescription::Constraint_Equals:
					if(value == constraint->Value) {
						output.push_back(util::Util::FormatBinary(value, chunk_length));
						// Only one output possible from this type of constraint
						return output;
					}
					break;
				case InstructionDescription::Constraint_BitwiseAnd:
					if((value & constraint->Value) == constraint->Value) {
						output.push_back(util::Util::FormatBinary(value, chunk_length));
					}
					break;
				case InstructionDescription::Constraint_BitwiseXor:
					if((value ^ constraint->Value) != 0) {
						output.push_back(util::Util::FormatBinary(value, chunk_length));
					}
					break;
				case InstructionDescription::Constraint_NotEquals:
					// These are EXTREMELY expensive to evaluate here so emit an unconstrained string here
					// and check it 'later'
					return { std::string(chunk_length, 'x') };
			}
		}
	}

	return output;
}

const std::vector<std::string> &InstructionDescription::GetBitString() const
{
	using gensim::util::Util;

	if (bitStringsCalculated) return bitStrings;
	bitStrings.clear();
	if (Decode_Constraints.empty()) {
		// Return the implicit unconstrained bit string
		std::stringstream str;
		char var_char[] = {'x', '\0'};
		for (std::list<InstructionFormatChunk>::const_iterator chunk = Format->GetChunks().begin(); chunk != Format->GetChunks().end(); ++chunk) {
			if (chunk->is_constrained) {
				str << Util::FormatBinary(chunk->constrained_value, chunk->length);
			} else {
				str << Util::StrDup(var_char, chunk->length);
			}
		}

		bitStrings.push_back(str.str());
	} else {
		// Build up bitstrings based on the decode constraints for the instruction
		//

		// We consider each set of decode constraints in turn
		for(const auto &constraint_set : Decode_Constraints) {
			// We're going to generate a map of chunks to possible values for those chunks,
			// and then permute the possible values per chunk to get the final bitstrings.
			std::unordered_map<const InstructionFormatChunk*, std::vector<std::string>> chunk_strings;

			// Now consider each chunk
			for(const auto &chunk : Format->GetChunks()) {
				// If this chunk is statically constrained, just give it it's statically constrained value
				if(chunk.is_constrained) {
					chunk_strings[&chunk] = {Util::FormatBinary(chunk.constrained_value, chunk.length)};
					continue;
				}

				// Collect all of the constraints placed on this chunk by the decode constraint set
				std::vector<const DecodeConstraint*> constraints;
				for(const auto &constraint : constraint_set) {
					if(constraint.Field == chunk.Name) constraints.push_back(&constraint);
				}

				// If we couldn't find any constraints for this chunk, just give it an all x value
				if(constraints.empty()) {
					chunk_strings[&chunk] = { std::string(chunk.length, 'x') };
					continue;
				}

				// Now generate the possible values for this chunk
				chunk_strings[&chunk] = ConstrainChunk(chunk.length, constraints);
			}

			// Loop over each chunk again.
			std::vector<std::string> constraintset_bitStrings = {""};

			for(const auto &chunk : Format->GetChunks()) {
				std::vector<std::string> new_bitstrings;
				for(const auto prefix : constraintset_bitStrings) {
					for(const auto suffix : chunk_strings.at(&chunk)) {
						new_bitstrings.push_back(prefix + suffix);
					}
				}

				constraintset_bitStrings.swap(new_bitstrings);
			}

			bitStrings.insert(bitStrings.begin(), constraintset_bitStrings.begin(), constraintset_bitStrings.end());

		}


	}
	return bitStrings;
}

bool InstructionDescription::load_constraints_from_node(gensim::AntlrTreeWrapper tree, std::list<std::vector<DecodeConstraint> > &target)
{
	pANTLR3_BASE_TREE node = tree.Get();
	bool success = true;
	bitStringsCalculated = false;
	// first node is identifier node - check that this instruction's name matches the node name
	std::vector<DecodeConstraint> node_constraints;
	pANTLR3_BASE_TREE nameNode = (pANTLR3_BASE_TREE)node->getChild(node, 0);

	assert(!(strcmp((char *)nameNode->getText(nameNode)->chars, this->Name.c_str())) && "Internal error: attempted to add constraints to non-matching instruction");

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
				node_constraints.push_back(DecodeConstraint::EqualsConstraint(fieldname, value));
				break;
			case AMPERSAND:
				node_constraints.push_back(DecodeConstraint::BitwiseAndConstraint(fieldname, value));
				break;
			case NOTEQUALS:
				node_constraints.push_back(DecodeConstraint::NotEqualsConstraint(fieldname, value));
				break;
			case XOR:
				node_constraints.push_back(DecodeConstraint::XOrConstraint(fieldname, value));
				break;
			default:
				assert(!"Unrecognized decode constraint operator!");
		}
	}
	target.push_back(node_constraints);

	return success;
}
