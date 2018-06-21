/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef VALUESET_H_
#define VALUESET_H_

#include <cstdint>
#include <map>
#include <random>
#include <unordered_set>

#include "TestHarnessSettings.h"

namespace gensim
{
	namespace arch
	{
		class ArchDescription;
	}
	namespace isa
	{
		class InstructionFormatChunk;
		class InstructionDescription;
	}

	namespace generator
	{
		namespace testgeneration
		{

			namespace ga
			{
				struct GAConfiguration;
			}

			class ConstraintSet;

			class FieldValue
			{
			public:
				FieldValue(uint32_t _mask = 0xffffffff);
				FieldValue(const isa::InstructionFormatChunk &chunk);

				inline bool operator==(const FieldValue &other) const
				{
					return mask == other.mask && value == other.value;
				}

				inline bool operator!=(const FieldValue &other) const
				{
					return !(operator==(other));
				}

				uint32_t mask;
				uint32_t value;
				bool fixed;

				void Mutate(std::mt19937 &r, const ga::GAConfiguration &config);
				static FieldValue Recombine(const FieldValue &m1, const FieldValue &m2, std::mt19937 &r);
				inline void Randomize(std::mt19937 &r)
				{
					if (fixed) return;
					value = r() & mask;
				}
			};

			class ValueSet
			{
			public:
				std::vector<FieldValue> Registers;
				std::vector<std::vector<FieldValue> > RegisterBanks;
				std::map<std::string, FieldValue> InsnFields;

				const isa::InstructionDescription *Insn;
				const arch::ArchDescription *Arch;

				ValueSet(const arch::ArchDescription *arch, const isa::InstructionDescription *insn);
				bool operator==(const ValueSet &other) const;

				void PopulateFields(const ConstraintSet &constraints);

				static ValueSet Recombine(const ValueSet &m1, const ValueSet &m2, ValueSet &out, std::mt19937 &r);
				void Mutate(std::mt19937 &r, const ga::GAConfiguration &config);

				void Randomize(std::mt19937 &r);
				void Zero();

				void PrettyPrint(std::ostream &str) const;

				// Special handling for PC since we shouldn't mutate/recombine this value
				void SetPC(uint32_t val)
				{
					RegisterBanks[0][15].value = val;
				}
				inline bool IsPC(uint32_t val) const
				{
					return val == 15;
				}
			};
		}
	}
}

namespace std
{
	template <>
	struct hash<gensim::generator::testgeneration::ValueSet> {
		uint32_t operator()(const gensim::generator::testgeneration::ValueSet &v) const
		{
			// sum register values
			uint32_t t;
			for (const auto &bank : v.RegisterBanks)
				for (const auto &reg : bank) t += reg.value;

			return t;
		}
	};
}

#endif /* VALUESET_H_ */
