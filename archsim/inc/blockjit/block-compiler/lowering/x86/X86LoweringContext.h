/*
 * X86LoweringContext.h
 *
 *  Created on: 16 Nov 2015
 *      Author: harry
 */

#ifndef INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86LOWERINGCONTEXT_H_
#define INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86LOWERINGCONTEXT_H_

#include "blockjit/block-compiler/lowering/LoweringContext.h"
#include "blockjit/block-compiler/lowering/InstructionLowerer.h"
#include "blockjit/encode.h"

#include "util/MemAllocator.h"

#include <map>
#include <vector>

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

					class X86LoweringContext : public LoweringContext
					{
					public:
						typedef captive::arch::x86::X86Encoder encoder_t;

						X86LoweringContext(uint32_t stack_frame_size, encoder_t &encoder);
						virtual ~X86LoweringContext();

						encoder_t &GetEncoder()
						{
							return _encoder;
						}

						virtual offset_t GetEncoderOffset() override;

						virtual bool Prepare(const TranslationContext &ctx, BlockCompiler &compiler) override;

						typedef std::map<const captive::arch::x86::X86Register*, uint32_t> stack_map_t;

						stack_map_t &GetStackMap()
						{
							return _stack_map;
						}
						bool &GetIsStackFixed()
						{
							return _stack_fixed;
						}

					protected:
						virtual bool LowerHeader(const TranslationContext &ctx) override;
						virtual bool PerformRelocations(const TranslationContext &ctx) override;

					private:
						stack_map_t _stack_map;
						captive::arch::x86::X86Encoder &_encoder;
						bool _stack_fixed;
					};

				}
			}
		}
	}
}



#endif /* INC_BLOCKJIT_BLOCK_COMPILER_LOWERING_X86LOWERINGCONTEXT_H_ */
