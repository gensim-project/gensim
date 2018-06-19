/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * Finalisation.h
 *
 *  Created on: 1 Dec 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_FINALISATION_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_FINALISATION_H_

#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "blockjit/block-compiler/lowering/x86/X86LoweringContext.h"

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
				
				namespace x86 {
					class X86LoweringContext;
				}
				
				class Finalisation
				{
				public:
					virtual ~Finalisation();
					virtual bool Finalise(LoweringContext &context) = 0;
				};
				
				class X86Finalisation : public Finalisation
				{
				public:
					virtual ~X86Finalisation() {}
					bool Finalise(LoweringContext &context);
					virtual bool FinaliseX86(x86::X86LoweringContext &ctx) = 0;
				};

			}
		}
	}
}


#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_FINALISATION_H_ */
