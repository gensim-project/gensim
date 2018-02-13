/*
 * GAConfig.h
 *
 *  Created on: 4 Nov 2013
 *      Author: harry
 */

#ifndef GACONFIG_H_
#define GACONFIG_H_

#include <limits>

namespace gensim
{
	namespace isa
	{
		class InstructionDescription;
	}
	namespace arch
	{
		class ArchDescription;
	}
	namespace generator
	{
		namespace testgeneration
		{
			namespace ga
			{

				struct GAConfiguration {
				public:
					uint32_t MaxNumGenerations;
					uint32_t GenerationSize;
					// Exressed out of INT_MAX - use SetMutationChancePerBit to set it to a float 0..1
					uint32_t MutateValueToZeroChance;
					uint32_t MutateValueToMaxChance;
					uint32_t MutateValueFlipBitsChance;
					uint32_t MutateValueIndividualBitFlipChance;

					uint32_t RecombinationMembers;

					uint32_t TerminateOnSolution;

					const arch::ArchDescription *Architecture;
					const isa::InstructionDescription *Instruction;

					inline void SetMutationZeroChance(float f);
					inline void SetMutationMaxChance(float f);
					inline void SetMutationFlipBitsChance(float f);
					inline void SetMutationChancePerBit(float f);

					inline uint32_t GetZeroThreshold() const;
					inline uint32_t GetOneThreshold() const;
					inline uint32_t GetFlipThreshold() const;

					inline uint32_t GetBitFlipThreshold() const;
				};

				uint32_t GAConfiguration::GetOneThreshold() const
				{
					return MutateValueToMaxChance;
				}

				uint32_t GAConfiguration::GetZeroThreshold() const
				{
					return MutateValueToMaxChance + MutateValueToZeroChance;
				}

				uint32_t GAConfiguration::GetFlipThreshold() const
				{
					return MutateValueToMaxChance + MutateValueToZeroChance + MutateValueFlipBitsChance;
				}

				uint32_t GAConfiguration::GetBitFlipThreshold() const
				{
					return MutateValueIndividualBitFlipChance;
				}

				void GAConfiguration::SetMutationZeroChance(float f)
				{
					MutateValueToZeroChance = f * (float)std::numeric_limits<uint32_t>::max();
				}

				void GAConfiguration::SetMutationMaxChance(float f)
				{
					MutateValueToMaxChance = f * (float)std::numeric_limits<uint32_t>::max();
				}

				void GAConfiguration::SetMutationFlipBitsChance(float f)
				{
					MutateValueFlipBitsChance = f * (float)std::numeric_limits<uint32_t>::max();
				}

				void GAConfiguration::SetMutationChancePerBit(float f)
				{
					MutateValueIndividualBitFlipChance = f * (float)std::numeric_limits<uint32_t>::max();
				}
			}
		}
	}
}

#endif /* GACONFIG_H_ */
