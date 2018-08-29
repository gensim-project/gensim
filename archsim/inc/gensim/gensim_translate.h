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
	class Module;
	class Function;
}

namespace archsim
{
	namespace ij
	{
		class IJTranslationContext;
	}
	namespace translate
	{
		namespace tx_llvm
		{
			class LLVMTranslationContext;
		}
	}
	namespace core
	{
		namespace thread
		{
			class ThreadInstance;
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

	class BaseLLVMTranslate
	{
	public:
		virtual bool TranslateInstruction(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, archsim::core::thread::ThreadInstance *thread, gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn) = 0;

		llvm::Value *EmitRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void *irbuilder, int size, int offset);
		bool EmitRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void *irbuilder, int size, int offset, llvm::Value*);

		llvm::Value *EmitMemoryRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void *irbuilder, int size, llvm::Value *address);
		void EmitMemoryWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void *irbuilder, int size, llvm::Value *address, llvm::Value *value);

	private:
		llvm::Value *GetRegisterPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, void *irbuilder, int size, int offset);
		llvm::Value *GetRegfilePtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx);
	};

	class BaseIJTranslate : public BaseTranslate
	{
	public:
		BaseIJTranslate(const gensim::Processor& cpu) : BaseTranslate(cpu) { }

		virtual bool TranslateInstruction(archsim::ij::IJTranslationContext& ctx, const gensim::BaseDecode& insn, uint32_t offset, bool trace) = 0;
	};
}

#endif /* _GENSIM_TRANSLATE_H */
