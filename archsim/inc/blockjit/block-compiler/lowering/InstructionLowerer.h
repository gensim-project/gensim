/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * InstructionLowerer.h
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_INSTRUCTIONLOWERER_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_INSTRUCTIONLOWERER_H_

#include "blockjit/IRInstruction.h"

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

				class LoweringContext;

				class InstructionLowerer
				{
				public:
					InstructionLowerer();
					virtual ~InstructionLowerer();

					virtual bool TxlnStart(captive::arch::jit::BlockCompiler *compiler, captive::arch::jit::TranslationContext *ctx);
					virtual bool Lower(const captive::shared::IRInstruction *&insn) = 0;

					void SetContexts(LoweringContext &lctx, const TranslationContext &txctx)
					{
						_lctx = &lctx;
						_txctx = &txctx;
					}

				protected:

					LoweringContext& GetLoweringContext()
					{
						return *_lctx;
					}
					const TranslationContext& GetTranslationContext()
					{
						return *_txctx;
					}

				private:
					LoweringContext *_lctx;
					const TranslationContext *_txctx;
				};

				class NOPInstructionLowerer : public InstructionLowerer
				{
				public:
					NOPInstructionLowerer();
					bool Lower(const captive::shared::IRInstruction *&insn) override;
				};

			}
		}
	}
}



#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_INSTRUCTIONLOWERER_H_ */
