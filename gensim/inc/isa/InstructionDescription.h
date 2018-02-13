/*
 * InstructionDescription.h
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#ifndef INC_ISA_INSTRUCTIONDESCRIPTION_H_
#define INC_ISA_INSTRUCTIONDESCRIPTION_H_

#include <cstdint>

#include <string>
#include <vector>

#include "util/AntlrWrapper.h"

#include "isa/AsmDescription.h"
#include "isa/InstructionFormatDescription.h"

namespace gensim
{
	namespace isa
	{

		class ISADescription;

		class InstructionDescription
		{
		public:
			enum DecodeConstraintType {
				Constraint_Equals,
				Constraint_NotEquals,
				Constraint_BitwiseAnd,
				Constraint_BitwiseXor
			};

			struct DecodeConstraint {
			public:
				std::string Field;
				DecodeConstraintType Type;
				uint32_t Value;

				DecodeConstraint(std::string field, DecodeConstraintType type, uint32_t value) : Field(field), Type(type), Value(value) {}

				static DecodeConstraint EqualsConstraint(std::string field, uint32_t value)
				{
					return DecodeConstraint(field, Constraint_Equals, value);
				}

				static DecodeConstraint BitwiseAndConstraint(std::string field, uint32_t value)
				{
					return DecodeConstraint(field, Constraint_BitwiseAnd, value);
				}

				static DecodeConstraint NotEqualsConstraint(std::string field, uint32_t value)
				{
					return DecodeConstraint(field, Constraint_NotEquals, value);
				}

				static DecodeConstraint XOrConstraint(std::string field, uint32_t value)
				{
					return DecodeConstraint(field, Constraint_BitwiseXor, value);
				}
			};

			std::string Name;

			std::string BehaviourName;

			const InstructionFormatDescription *Format;
			std::list<std::vector<DecodeConstraint> > Decode_Constraints;
			std::list<std::vector<DecodeConstraint> > EOB_Contraints;

			std::list<std::vector<DecodeConstraint> > Uses_PC_Constraints;
			std::vector<uint32_t> Specialisations;

			std::list<AsmDescription*> Disasms;

			bool VariableJump;
			bool FixedJump, FixedJumpPred;
			uint8_t FixedJumpType;
			bool blockCond = false;
			signed int FixedJumpOffset;
			std::string FixedJumpField;

			const isa::ISADescription &ISA;

			uint8_t LimmCount;

			InstructionDescription(const isa::ISADescription &isa) : Name("Invalid Instruction"), Format(NULL), VariableJump(false), FixedJump(false), FixedJumpPred(false), LimmCount(0), bitStringsCalculated(false), ISA(isa) {}

			InstructionDescription(std::string name, const isa::ISADescription &isa, InstructionFormatDescription *format) : VariableJump(false), FixedJump(false), FixedJumpPred(false), LimmCount(0), bitStringsCalculated(false), ISA(isa)
			{
				Name = name;
				Format = format;
				BehaviourName = name;
				format->RegisterInstruction(*this);
			}

			const std::vector<std::string> &GetBitString() const;

			bool load_constraints_from_node(gensim::AntlrTreeWrapper tree, std::list<std::vector<DecodeConstraint> > &target);

		private:
			mutable bool bitStringsCalculated;
			mutable std::vector<std::string> bitStrings;
		};

		class InstructionDescriptionParser
		{
		public:
			InstructionDescriptionParser(DiagnosticContext &diag);

			bool Parse(gensim::AntlrTreeWrapper node, const ISADescription &ISA);
			InstructionDescription *Get();

		private:
			DiagnosticContext &diag;
			InstructionDescription *description;

		};


	}
}



#endif /* INC_ISA_INSTRUCTIONDESCRIPTION_H_ */
