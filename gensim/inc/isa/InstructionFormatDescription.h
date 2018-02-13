/*
 * InstructionFormatDescription.h
 *
 *  Created on: 20 May 2015
 *      Author: harry
 */

#ifndef INC_ARCH_INSTRUCTIONFORMATDESCRIPTION_H_
#define INC_ARCH_INSTRUCTIONFORMATDESCRIPTION_H_

#include <cassert>
#include <cstdint>

#include <list>
#include <string>

#include "DiagnosticContext.h"

namespace gensim
{
	namespace isa
	{
		class ISADescription;
		class InstructionDescription;

		struct InstructionFormatChunk {
			/*
			 * The name used to refer to this chunk in the format specifier and when referring to it as a field
			 */
			std::string Name;

			/*
			 * True if this field of this format must hold a specific value
			 */
			bool is_constrained;

			/*
			 * Length of this chunk in bits
			 */
			uint8_t length;

			/*
			 * True if this chunk should be sign extended
			 */
			bool is_signed;

			/*
			 * True if this chunk should have a field generated in any struct representing an instruction
			 */
			bool generate_field;

			/*
			 * If this field is constrained, this contains the value the field should be constrained to
			 */
			uint32_t constrained_value;

			InstructionFormatChunk(char const *const _name, uint8_t _length, bool _is_signed, bool _generate_field) : is_constrained(false), length(_length), is_signed(_is_signed), generate_field(_generate_field)
			{
				Name.assign(_name);
			}
		};

		class InstructionFormatDescription
		{
		public:
			typedef std::list<InstructionFormatChunk> ChunkList;
			typedef std::list<InstructionDescription *> InstructionList;
			typedef std::list<std::string> AttributeList;

			void SetName(std::string newname)
			{
				Name = newname;
			}
			const std::string &GetName() const
			{
				return Name;
			}

			std::string DecodeBehaviourCode;

			std::string ToString();

			bool hasChunk(std::string name) const;

			void AddChunk(const InstructionFormatChunk &chunk)
			{
				Chunks.push_back(chunk);
				Length += chunk.length;
			}

			const ChunkList &GetChunks() const
			{
				return Chunks;
			}

			const InstructionList &GetInstructions() const
			{
				return Instructions;
			}

			void RegisterInstruction(InstructionDescription &insn)
			{
				Instructions.push_back(&insn);
			}

			const InstructionFormatChunk &GetChunkByName(std::string) const;

			uint32_t GetLength() const
			{
				return Length;
			}

			bool CanBePredicated() const
			{
				return can_be_predicated;
			}
			void SetCanBePredicated(bool cbp)
			{
				can_be_predicated = cbp;
			}

			const ISADescription& GetISA() const
			{
				return isa;
			}

			void AddAttribute(const std::string& attr)
			{
				Attributes.push_back(attr);
			}

			bool HasAttribute(const std::string& check_attr) const
			{
				for (const auto& attr : Attributes) {
					if (attr == check_attr) return true;
				}

				return false;
			}

			InstructionFormatDescription(const ISADescription& isa) : Name("Invalid Format"), Length(0), can_be_predicated(true), isa(isa) {}

		private:
			std::string Name;

			uint32_t Length;
			ChunkList Chunks;
			InstructionList Instructions;
			AttributeList Attributes;

			bool can_be_predicated;
			const ISADescription& isa;

			InstructionFormatDescription(const InstructionFormatDescription &orig) = delete;
		};

		class InstructionFormatDescriptionParser
		{
		public:
			InstructionFormatDescriptionParser(DiagnosticContext &diag, const ISADescription& isa);

			bool Parse(const std::string &name, const std::string &format_str, InstructionFormatDescription *&desc);
			
		private:
			DiagnosticContext &diag;
			const ISADescription &isa_;
		};

	}
}


#endif /* INC_ARCH_INSTRUCTIONFORMATDESCRIPTION_H_ */
