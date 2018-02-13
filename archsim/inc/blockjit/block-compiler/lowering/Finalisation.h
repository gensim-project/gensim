/*
 * Finalisation.h
 *
 *  Created on: 1 Dec 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_FINALISATION_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_FINALISATION_H_


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

				class Finalisation
				{
				public:
					virtual ~Finalisation();
					virtual bool Finalise(LoweringContext &context) = 0;
				};

			}
		}
	}
}


#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_FINALISATION_H_ */
