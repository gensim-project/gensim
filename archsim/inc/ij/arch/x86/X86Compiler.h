/*
 * File:   X86Compiler.h
 * Author: s0457958
 *
 * Created on 15 September 2014, 17:05
 */

#ifndef X86COMPILER_H
#define	X86COMPILER_H

#include "ij/IJCompiler.h"
#include "ij/ir/IRInstruction.h"
#include "X86Support.h"

namespace archsim
{
	namespace ij
	{
		class IJIRBlock;

		namespace ir
		{
			class IRValue;
		}

		namespace arch
		{
			namespace x86
			{
				class X86OutputBuffer;
				class X86Register;

				class X86Compiler : public IJCompiler
				{
				public:
					uint32_t Compile(IJIR& ir, uint8_t* buffer) override;

				private:
					uint8_t *LowerBlock(X86OutputBuffer& buffer, IJIRBlock& block);

					const X86Register& ValueToRegister(ir::IRValue *val);
					const X86Immediate ValueToImmediate(ir::IRValue *val);

					bool LowerInstruction(X86OutputBuffer& buffer, ir::IRInstruction& insn);
					bool LowerRetInstruction(X86OutputBuffer& buffer, ir::IRRetInstruction& insn);
					bool LowerArithmeticInstruction(X86OutputBuffer& buffer, ir::IRArithmeticInstruction& insn);
				};
			}
		}
	}
}

#endif	/* X86COMPILER_H */

