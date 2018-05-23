/*
 * X86Lowerers.h
 *
 *  Created on: 3 Dec 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86_X86LOWERERS_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86_X86LOWERERS_H_

#include "util/LogContext.h"

UseLogContext(LogBlockJit);
#define CANTLOWER do { LC_ERROR(LogBlockJit) << "Can't lower in " << __FILE__ << ":" << __LINE__; return false; } while(0)

namespace captive
{
	namespace arch
	{
		namespace jit
		{
			namespace lowering
			{
				namespace x86
				{

					class X86InstructionLowerer : public InstructionLowerer
					{
					public:
						X86Encoder &Encoder()
						{
							return ((X86LoweringContext&)GetLoweringContext()).GetEncoder();
						}

						bool &GetIsStackFixed()
						{
							return ((X86LoweringContext&)GetLoweringContext()).GetIsStackFixed();
						}
						X86LoweringContext::stack_map_t &GetStackMap()
						{
							return ((X86LoweringContext&)GetLoweringContext()).GetStackMap();
						}
						
						X86LoweringContext& GetLoweringContext()
						{
							return (X86LoweringContext&)InstructionLowerer::GetLoweringContext();
						}
					};

					class LowerReadMemUser : public X86InstructionLowerer
					{
					public:
						virtual ~LowerReadMemUser()
						{

						}

						bool Lower(const captive::shared::IRInstruction*& insn) override;
						bool TxlnStart(captive::arch::jit::BlockCompiler *compiler, captive::arch::jit::TranslationContext *ctx) override;

					private:
						shared::IRRegId mem_base_;
					};

					class LowerWriteMemUser : public X86InstructionLowerer
					{
					public:
						virtual ~LowerWriteMemUser()
						{

						}

						bool Lower(const captive::shared::IRInstruction*& insn) override;
						bool TxlnStart(captive::arch::jit::BlockCompiler *compiler, captive::arch::jit::TranslationContext *ctx) override;

					private:
						shared::IRRegId mem_base_;
					};

#define LowerType(X) class Lower ## X : public X86InstructionLowerer { public: Lower ## X() : X86InstructionLowerer() {} bool Lower(const captive::shared::IRInstruction *&insn) override; };
#define LowerTypeTS(X) class Lower ## X;
#include "blockjit/block-compiler/lowering/x86/LowerTypes.h"
#undef LowerType
#undef LowerTypeTS
				}
			}
		}
	}
}


#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86_X86LOWERERS_H_ */
