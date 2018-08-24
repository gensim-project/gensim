/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   gensim_translate.h
 * Author: s0803652
 *
 * Created on 07 November 2011, 14:55
 */

#ifndef _GENSIM_TRANSLATE_H
#define _GENSIM_TRANSLATE_H

#include "../define.h"
#include "../abi/Address.h"
#include "gensim_decode.h"

#include <string>

namespace llvm
{
	class Value;
}

namespace archsim
{
	namespace ij
	{
		class IJTranslationContext;
	}
	namespace translate
	{
		namespace translate_llvm
		{
			class LLVMTranslationContext;
			class LLVMInstructionTranslationContext;
		}
	}
}

namespace gensim
{
	class Processor;

	class JumpInfo
	{
	public:
		bool IsJump;
		bool IsIndirect;
		bool IsConditional;
		archsim::Address JumpTarget;
	};

	class BaseJumpInfoProvider
	{
	public:
		virtual ~BaseJumpInfoProvider() {}
		virtual void GetJumpInfo(const gensim::BaseDecode *instr, archsim::Address pc, JumpInfo &info) {}
	};

	class BaseTranslate
	{
	public:
		BaseTranslate(const gensim::Processor& cpu) : cpu(cpu) { }
		virtual ~BaseTranslate() { }

		virtual void GetJumpInfo(const gensim::BaseDecode *instr, uint32_t pc, bool& indirect_jump, bool& direct_jump, uint32_t& jump_target) = 0;

	protected:
		const gensim::Processor &cpu;
	};

	class BaseLLVMTranslate : public BaseTranslate
	{
	public:
		BaseLLVMTranslate(const gensim::Processor& cpu, archsim::translate::translate_llvm::LLVMTranslationContext& ctx) : BaseTranslate(cpu), txln_ctx(ctx) { }

		virtual uint8_t *GetPrecompBitcode() = 0;
		virtual uint32_t GetPrecompSize() = 0;

		virtual bool EmitPredicate(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& ctx, ::llvm::Value*& __result, bool trace) = 0;
		virtual bool TranslateInstruction(archsim::translate::translate_llvm::LLVMInstructionTranslationContext& ctx, ::llvm::Value*& __result, bool trace) = 0;

	protected:
		archsim::translate::translate_llvm::LLVMTranslationContext& txln_ctx;
	};

	class BaseIJTranslate : public BaseTranslate
	{
	public:
		BaseIJTranslate(const gensim::Processor& cpu) : BaseTranslate(cpu) { }

		virtual bool TranslateInstruction(archsim::ij::IJTranslationContext& ctx, const gensim::BaseDecode& insn, uint32_t offset, bool trace) = 0;
	};
}

#endif /* _GENSIM_TRANSLATE_H */
