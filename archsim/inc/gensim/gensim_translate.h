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
#include "gensim_processor_api.h"

#include <string>
#include <map>
#include <list>

namespace llvm
{
	class Value;
	class Module;
	class Function;
	class BasicBlock;
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
		virtual bool TranslateInstruction(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, archsim::core::thread::ThreadInstance *thread, const gensim::BaseDecode *decode, archsim::Address phys_pc, llvm::Function *fn) = 0;

		llvm::Value *EmitRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset);
		bool EmitRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset, llvm::Value*);

		void EmitTraceRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value *value);
		void EmitTraceBankedRegisterWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value *regnum, int size, llvm::Value *value_ptr);
		void EmitTraceRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value *value);
		void EmitTraceBankedRegisterRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int id, llvm::Value *regnum, int size, llvm::Value *value_ptr);

		llvm::Value *EmitMemoryRead(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value *address);
		void EmitMemoryWrite(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int interface, int size_in_bytes, llvm::Value *address, llvm::Value *value);

		void EmitTakeException(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, llvm::Value *category, llvm::Value *data);

		void EmitAdcWithFlags(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int bits, llvm::Value *lhs, llvm::Value *rhs, llvm::Value *carry);
		void EmitSbcWithFlags(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int bits, llvm::Value *lhs, llvm::Value *rhs, llvm::Value *carry);

	protected:
		void QueueDynamicBlock(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, std::map<uint16_t, llvm::BasicBlock*> &dynamic_blocks, std::list<uint16_t> &dynamic_block_queue, uint16_t queued_block);

	private:
		llvm::Value *GetRegisterPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx, int size_in_bytes, int offset);
		llvm::Value *GetRegfilePtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx);
		llvm::Value *GetThreadPtr(archsim::translate::tx_llvm::LLVMTranslationContext& ctx);
	};

	class BaseIJTranslate : public BaseTranslate
	{
	public:
		BaseIJTranslate(const gensim::Processor& cpu);

		virtual bool TranslateInstruction(archsim::ij::IJTranslationContext& ctx, const gensim::BaseDecode& insn, uint32_t offset, bool trace) = 0;
	};
}

#endif /* _GENSIM_TRANSLATE_H */
