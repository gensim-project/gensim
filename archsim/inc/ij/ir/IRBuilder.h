/*
 * File:   IRBuilder.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 15:52
 */

#ifndef IRBUILDER_H
#define	IRBUILDER_H

#include "ij/IJIR.h"
#include "ij/ir/IRValue.h"
#include "ij/ir/IRInstruction.h"

namespace archsim
{
	namespace ij
	{
		class IJIR;
		class IJIRBlock;

		namespace ir
		{
			class IRValue;

			class IRBuilder
			{
			public:
				IRBuilder(IJIR& ir, IJIRBlock& block);

				void SetInsertPoint(IJIRBlock& block)
				{
					current_block = &block;
				}
				IJIRBlock& GetInsertPoint() const
				{
					return *current_block;
				}

				inline IRRetInstruction *CreateRet(IRValue *result)
				{
					return (IRRetInstruction *)current_block->AddInstruction(new IRRetInstruction(*current_block, result));
				}

				inline IRArithmeticInstruction *CreateAdd(IRValue *source, IRValue *dest)
				{
					return (IRArithmeticInstruction *)current_block->AddInstruction(new IRArithmeticInstruction(*current_block, source, dest, IRArithmeticInstruction::Add));
				}
				inline IRArithmeticInstruction *CreateSub(IRValue *source, IRValue *dest)
				{
					return (IRArithmeticInstruction *)current_block->AddInstruction(new IRArithmeticInstruction(*current_block, source, dest, IRArithmeticInstruction::Subtract));
				}
				inline IRArithmeticInstruction *CreateMul(IRValue *source, IRValue *dest)
				{
					return (IRArithmeticInstruction *)current_block->AddInstruction(new IRArithmeticInstruction(*current_block, source, dest, IRArithmeticInstruction::Multiply));
				}
				inline IRArithmeticInstruction *CreateDiv(IRValue *source, IRValue *dest)
				{
					return (IRArithmeticInstruction *)current_block->AddInstruction(new IRArithmeticInstruction(*current_block, source, dest, IRArithmeticInstruction::Divide));
				}
				inline IRArithmeticInstruction *CreateMod(IRValue *source, IRValue *dest)
				{
					return (IRArithmeticInstruction *)current_block->AddInstruction(new IRArithmeticInstruction(*current_block, source, dest, IRArithmeticInstruction::Modulo));
				}

			private:
				IJIR& ir;
				IJIRBlock *current_block;
			};
		}
	}
}

#endif	/* IRBUILDER_H */

