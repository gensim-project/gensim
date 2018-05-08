/*
 * InstructionDescription.cpp
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#include "isa/InstructionDescription.h"
#include "Util.h"

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
