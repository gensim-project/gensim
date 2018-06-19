/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * LoweringContext.h
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_LOWERINGCONTEXT_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_LOWERINGCONTEXT_H_

#include "blockjit/IRInstruction.h"
#include "blockjit/translation-context.h"
#include "blockjit/block-compiler/analyses/Analysis.h"
#include <map>
#include <vector>

namespace archsim {
	class ArchDescriptor;
	class StateBlockDescriptor;
}

namespace captive
{
	namespace arch
	{
		namespace jit
		{

			class TranslationContext;
			class BlockCompiler;

			namespace lowering
			{

				class InstructionLowerer;
				class Finalisation;

				class LoweringContext
				{
				public:
					LoweringContext(const archsim::ArchDescriptor &arch, const archsim::StateBlockDescriptor &sbd);
					virtual ~LoweringContext();
					
					virtual bool Prepare(const TranslationContext &ctx) = 0;
					bool PrepareLowerers(const TranslationContext &ctx);

					virtual bool Lower(const TranslationContext &ctx);
					
					const archsim::ArchDescriptor &GetArchDescriptor() const { return arch_descriptor_; }
					const archsim::StateBlockDescriptor &GetStateBlockDescriptor() const { return sb_descriptor_; }
					
				protected:
					virtual bool LowerHeader(const TranslationContext &ctx) = 0;
					virtual bool LowerBody(const TranslationContext &ctx);
					virtual bool LowerBlock(const TranslationContext &ctx, captive::shared::IRBlockId block_id, uint32_t block_start);
					virtual bool LowerInstruction(const TranslationContext &ctx, const captive::shared::IRInstruction *&insn);
	
					bool AddLowerer(captive::shared::IRInstruction::IRInstructionType type, InstructionLowerer* lowerer);
					
				private:
					std::vector<InstructionLowerer*> _lowerers;
					
					const archsim::ArchDescriptor &arch_descriptor_;
					const archsim::StateBlockDescriptor &sb_descriptor_;
				};
				
				class MCLoweringContext : public LoweringContext
				{
				public:
					MCLoweringContext(uint32_t stack_frame_size, const archsim::ArchDescriptor &arch, const archsim::StateBlockDescriptor &sbd);
					virtual ~MCLoweringContext();

					typedef size_t offset_t;
					bool RegisterBlockOffset(captive::shared::IRBlockId block, offset_t offset);
					bool RegisterBlockRelocation(offset_t offset, shared::IRBlockId block);

					bool RegisterFinalisation(Finalisation *f);
					bool PerformFinalisations();

					virtual size_t GetEncoderOffset() = 0;
					virtual uint32_t GetStackFrameSize()
					{
						uint32_t returned_size = _stack_frame_size;
						if(returned_size & 15) {
							returned_size = (returned_size & ~15) + 16;
						}
						return returned_size;
					}
					virtual void SetStackFrameSize(uint32_t a)
					{
						_stack_frame_size = a;
					}

					virtual bool Lower(const TranslationContext &ctx) override;

				protected:

					virtual bool LowerBlock(const TranslationContext &ctx, captive::shared::IRBlockId block_id, uint32_t block_start);

					virtual bool PerformRelocations(const TranslationContext &ctx) = 0;

					typedef std::pair<offset_t, shared::IRBlockId> block_relocation_t;
					const std::vector<block_relocation_t> &GetBlockRelocations() const
					{
						return _block_relocations;
					}
					offset_t GetBlockOffset(shared::IRBlockId block) const
					{
						return _block_offsets.at(block);
					}

				private:
					std::vector<block_relocation_t> _block_relocations;
					std::vector<offset_t> _block_offsets;

					std::vector<Finalisation*> _finalisations;

					analyses::HostRegLivenessData _liveness;

					uint32_t _stack_frame_size;
				};

			}
		}
	}
}



#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_LOWERINGCONTEXT_H_ */
